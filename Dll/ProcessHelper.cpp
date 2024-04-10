#include "pch.h"
#include "ProcessHelper.h"


int EnableSeDebugPrivilege(HANDLE ProcessHandle, BOOL IsEnable, LPCTSTR RequireLevel)
{
	DWORD  LastError;
	HANDLE TokenHandle = 0;
	//TOKEN_ADJUST_PRIVILEGES 表示函数调用者想要调整访问令牌的特权。
	//TOKEN_QUERY 表示函数调用者想要获取访问令牌的信息。
	if (!OpenProcessToken(ProcessHandle, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &TokenHandle))
	{
		LastError = GetLastError();
		if (TokenHandle)
			CloseHandle(TokenHandle);
		return LastError;
	}
	TOKEN_PRIVILEGES TokenPrivileges;
	memset(&TokenPrivileges, 0, sizeof(TOKEN_PRIVILEGES));
	LUID v1;
	//LUID用来保存特权的id,若LookupPrivilegeValue调用成功，v1将会接受该特权的LUID
	if (!LookupPrivilegeValue(NULL, RequireLevel, &v1))
	{
		LastError = GetLastError();
		CloseHandle(TokenHandle);
		return LastError;
	}
	TokenPrivileges.PrivilegeCount = 1;
	TokenPrivileges.Privileges[0].Luid = v1;
	//上文代码先找到SHUTDOWN权限的LUID
	//下文代码对SHUTDOWN权限进行打开或关闭的操作
	//SE_PRIVILEGE_ENABLED代表打开该权限，0代表关闭该权限
	if (IsEnable)
		TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		TokenPrivileges.Privileges[0].Attributes = 0;
	//将TokenHandle所对应进程的特权改为TokenPrrivileges所寄存的状态
	AdjustTokenPrivileges(TokenHandle, FALSE, &TokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
	LastError = GetLastError();
	CloseHandle(TokenHandle);
	return LastError;
}

BOOL EnableDebugPrivilege(IN const TCHAR* PriviledgeName, BOOL IsEnable)
{
	// 打开权限令牌

	HANDLE  ProcessHandle = GetCurrentProcess();   //获得当前自己的进程句柄
	HANDLE  TokenHandle = NULL;

	//通过进程句柄获得进程令牌句柄
	if (!OpenProcessToken(ProcessHandle, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &TokenHandle))  //
	{
		return FALSE;
	}
	LUID			 v1;
	if (!LookupPrivilegeValue(NULL, PriviledgeName, &v1))		// 通过权限名称查找uID
	{
		CloseHandle(TokenHandle);
		TokenHandle = NULL;
		return FALSE;
	}
	TOKEN_PRIVILEGES TokenPrivileges = { 0 };
	TokenPrivileges.PrivilegeCount = 1;		// 要提升的权限个数
	TokenPrivileges.Privileges[0].Attributes = IsEnable == TRUE ? SE_PRIVILEGE_ENABLED : 0;    // 动态数组，数组大小根据Count的数目
	TokenPrivileges.Privileges[0].Luid = v1;
	if (!AdjustTokenPrivileges(TokenHandle, FALSE, &TokenPrivileges,
		sizeof(TOKEN_PRIVILEGES), NULL, NULL))
	{

		CloseHandle(TokenHandle);
		TokenHandle = NULL;
		return FALSE;
	}
	CloseHandle(TokenHandle);
	TokenHandle = NULL;
	return TRUE;
}


//判断进程位数

//注意ISWow64代表的是64位下的32位
BOOL XkIsWow64Process(HANDLE ProcessHandle, BOOL* isWow64Process)
{
	//获得ntdll模块的函数
	HMODULE	Kernel32ModuleBase = NULL;

	Kernel32ModuleBase = GetModuleHandle(_T("kernel32.dll"));
	if (Kernel32ModuleBase == NULL)
	{
		return FALSE; 
	}
	typedef BOOL(__stdcall* LPFN_ISWOW64PROCESS)(HANDLE ProcessHandle, BOOL* isWow64Process);
	LPFN_ISWOW64PROCESS  isWow64 = NULL;
	isWow64 = (LPFN_ISWOW64PROCESS)GetProcAddress(Kernel32ModuleBase, "IsWow64Process");

	if (isWow64 == NULL)
	{
		return FALSE;
	}
	return isWow64(ProcessHandle, isWow64Process);
//Exit:

	//GetModuleHandle获取的模块句柄不应该用FreeLibrary来释放，FreeLibrary应该用来和
	//LoadLibrary 对应


	/*
	GetModuleHandle获取的模块句柄不需要手动释放
	*/

	//if (Kernel32ModuleBase != NULL)
	//{
	//	FreeLibrary(Kernel32ModuleBase);
	//	Kernel32ModuleBase = NULL;
	//}



	//return TRUE;
}

//加一个扩展库 Psapi
// 加载TlHelp32
//捕获进程快照
BOOL EnumProcessByToolHelp32(vector<PROCESS_INFORMATION_ITEM>& ProcessInfo)
{

	BOOL v1 = FALSE;
	HANDLE   SnapshotHandle = NULL;
	HANDLE   ProcessHandle = NULL;
	char     isWow64Process[20] = { 0 };
	PROCESSENTRY32  ProcessEntry32;   //官方
	PROCESS_INFORMATION_ITEM    v2 = { 0 }; //自定义
	TCHAR  ProcessFullPath[MAX_PATH] = { 0 };
	HMODULE ModuleHandle = NULL;
	DWORD ReturnLength = 0;
	ProcessEntry32.dwSize = sizeof(PROCESSENTRY32);

	//快照句柄		//加载TlHelp32
	SnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	//快照句柄作为资源的引用，而不是资源本身
	//要遍历快照信息仍需要一系列的API函数来进行操作
	//如Process32First   Process32Next


	if (SnapshotHandle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	//得到第一个进程顺便判断一下 获取系统快照是否成功
	if (Process32First(SnapshotHandle, &ProcessEntry32))
	{
		do
		{
			//打开进程并返回句柄  //4 system
			//查询信息和读取虚拟内存的权限
			ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
				FALSE, ProcessEntry32.th32ProcessID);   //打开目标进程  
				//ProcessHandle为空说明打开失败,权限不足,所以尝试用更低的权限打开
			if (ProcessHandle == NULL)
			{
				ProcessHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
					FALSE, ProcessEntry32.th32ProcessID);   //打开目标进程

				if (ProcessHandle == NULL)
				{
					//无法打开目标进程，自然无法判断进程的位数
					memcpy(ProcessFullPath, _T("打开进程失败"), _tcslen(_T("打开进程失败")));

					memcpy(isWow64Process, _T("无法判断"), _tcslen(_T("无法判断")));
					goto Label;

				}

			}
			//判断目标进程的位数

			if (XkIsWow64Process(ProcessHandle, &v1) == TRUE)
			{
				if (v1)
				{
					memcpy(isWow64Process, _T("32位"), _tcslen(_T("32位")));
				}
				else
				{
					memcpy(isWow64Process, _T("64位"), _tcslen(_T("64位")));
				}
			}
			else
			{
				memcpy(isWow64Process, _T("无法判断"), _tcslen(_T("无法判断")));
			}

			//通过进程句柄获得第一个模块句柄信息

			//加一个扩展库 Psapi
			//ModuleHandle此时是空,该参数是一个输入参数,代表了想要获取完全限定路径的模块
			//如果ModuleHandle为空，那么函数将返回进程的可执行文件的路径

			ReturnLength = GetModuleFileNameEx(ProcessHandle, ModuleHandle,
				ProcessFullPath,
				sizeof(ProcessFullPath));

			if (ReturnLength == 0)
			{
				//如果失败
				RtlZeroMemory(ProcessFullPath, MAX_PATH);

				QueryFullProcessImageName(ProcessHandle, 0, ProcessFullPath, &ReturnLength);	// 更推荐使用这个函数
				if (ReturnLength == 0)
				{
					memcpy(ProcessFullPath, _T("枚举信息失败"), _tcslen(_T("枚举信息失败")));
				}
			}

		Label:
			ZeroMemory(&v2, sizeof(v2));

			v2.ProcessIdentity = (HANDLE)ProcessEntry32.th32ProcessID;
			memcpy(v2.ProcessImageName, ProcessEntry32.szExeFile, (_tcslen(ProcessEntry32.szExeFile) + 1) * sizeof(TCHAR));
			memcpy(v2.ProcessFullPath, ProcessFullPath, (_tcslen(ProcessFullPath) + 1) * sizeof(TCHAR));
			memcpy(v2.isWow64Process, isWow64Process, (_tcslen(isWow64Process) + 1) * sizeof(TCHAR));
			ProcessInfo.push_back(v2);

			if (ProcessHandle != NULL)
			{
				CloseHandle(ProcessHandle);
				ProcessHandle = NULL;
			}

		} while (Process32Next(SnapshotHandle, &ProcessEntry32));
	}
	else
	{
		CloseHandle(SnapshotHandle);

		return FALSE;
	}

	CloseHandle(SnapshotHandle);

	return ProcessInfo.size() > 0 ? TRUE : FALSE;
}

VOID KillProcess(LPBYTE BufferData, UINT BufferLength)
{
	HANDLE ProcessHandle = NULL;
	EnableSeDebugPrivilege(GetCurrentProcess(), TRUE, SE_DEBUG_NAME);;  //提权

	for (int i = 0; i < BufferLength; i += sizeof(HANDLE))
		//因为结束的可能个不止是一个进程
	{
		//打开进程
		ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, *(LPDWORD)(BufferData + i));
		//结束进程
		TerminateProcess(ProcessHandle, 0);
		CloseHandle(ProcessHandle);
	}
	EnableSeDebugPrivilege(GetCurrentProcess(), FALSE, SE_DEBUG_NAME);;  //还原提权
	// 稍稍Sleep下，防止出错
	Sleep(100);

}

BOOL OpenProcessByProcessID(HANDLE ProcessIdentity, HANDLE* ProcessHandle)
{
	if (IsValidWritePoint(ProcessHandle) == FALSE)
	{
		return FALSE;
	}
	//提权
	if (EnableDebugPrivilege(_T("SeDebugPrivilege"), TRUE) == FALSE)
	{
		return FALSE;
	}

	//打开目标进程获得目标进程句柄
	*ProcessHandle = OpenProcess(GENERIC_ALL, FALSE, (DWORD)ProcessIdentity);

	if (*ProcessHandle != INVALID_HANDLE_VALUE)
	{
		EnableDebugPrivilege(_T("SeDebugPrivilege"), FALSE);
		return TRUE;
	}
	EnableDebugPrivilege(_T("SeDebugPrivilege"), FALSE);
	return FALSE;
}