#include "pch.h"
#include "InstantMessageManager.h"
#include "Common.h"

CHAR   __BufferData[0x1000] = { 0 };
CIocpClient* __IocpClient = NULL;
int    __TimeEvent = 0;

#define WINDOW_WIDTH		220   
#define WINDOW_HEIGHT		150
#define ID_TIMER_POP_WINDOW		1
#define ID_TIMER_DELAY_DISPLAY	2 
#define ID_TIMER_CLOSE_WINDOW	3 
CInstantMessageManager::CInstantMessageManager(CIocpClient* IocpClient) :CManager(IocpClient)
{
	//回传数据包到服务器
	BYTE	IsToken = CLIENT_REMOTE_MESSAGE_REPLY;
	IocpClient->OnSending((char*)&IsToken, 1);

	__IocpClient = IocpClient;   //不应该的  消息回调  
	//该函数是父类中实现
	WaitingForDialogOpen();   //等待服务器执行
}
CInstantMessageManager::~CInstantMessageManager()
{
	_tprintf(_T("~CRemoteMessageManage()\r\n"));
}

void CInstantMessageManager::HandleIo(PBYTE BufferData, ULONG_PTR BufferLength)
{

	switch (BufferData[0])
	{
	case CLIENT_GO_ON:
	{
		//该函数是父类中实现
		NotifyDialogIsOpen();
		break;
	}
	default:
	{
		//获得远程消息的数据内容
		memcpy(__BufferData, BufferData, BufferLength);

		//构建一个Dialog类
		DialogBoxA(__InstanceHandle, MAKEINTRESOURCE(IDD_INSTANT_MESSAGE_DIALOG),
			NULL, DialogProcedure);  //SDK   C   MFC  C++
		break;
	}
	}
}

int CALLBACK DialogProcedure(HWND DialogHwnd, unsigned int Message, WPARAM ParameterData1, LPARAM ParameterData2)
{

	switch (Message)
	{
	case WM_INITDIALOG:
	{
		OnInitDialog(DialogHwnd);
		break;
	}
	case WM_TIMER:
	{
		OnTimerDialog(DialogHwnd);
		break;
	}

	}

	return 0;   //false  一定要注意
}

VOID OnInitDialog(HWND DialogHwnd)
{
	MoveWindow(DialogHwnd, 0, 0, 0, 0, TRUE);

	//在IDC_EDIT_DIALOG_REMOTE_MESSAGE_MANAGER_MAIN控件上设置数据
	SetDlgItemText(DialogHwnd, IDC_INSTANT_MESSAGE_EDIT, __BufferData);

	memset(__BufferData, 0, sizeof(__BufferData));

	__TimeEvent = ID_TIMER_POP_WINDOW;    //时钟事件
	//设置时钟
	SetTimer(DialogHwnd, __TimeEvent, 1, NULL);

	PlaySound(MAKEINTRESOURCE(IDR_WAVE),
		__InstanceHandle, SND_ASYNC | SND_RESOURCE | SND_NODEFAULT);
}


VOID OnTimerDialog(HWND DialogHwnd)   //时钟回调
{
	RECT  Rect;
	static int Height = 0;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &Rect, 0);
	int y = Rect.bottom - Rect.top;
	//int x = Rect.right - Rect.left ;
	//x = x - WIN_WIDTH;            

	int x = 0;
	switch (__TimeEvent)
	{
	case ID_TIMER_CLOSE_WINDOW:
	{
		if (Height >= 0)
		{
			Height -= 5;
			MoveWindow(DialogHwnd, x, y - Height, WINDOW_WIDTH, Height, TRUE);
		}
		else
		{
			KillTimer(DialogHwnd, ID_TIMER_CLOSE_WINDOW);

			//通知服务器可以继续发送新的数据
			//BYTE IsToken = CLIENT_REMOTE_MESSAGE_COMPLETE;			   
			//__IOCPClient->OnSending((char*)&IsToken, 1);

			//关闭当前对话框
			EndDialog(DialogHwnd, 0);
		}
		break;
	}

	case ID_TIMER_DELAY_DISPLAY:
	{
		KillTimer(DialogHwnd, ID_TIMER_DELAY_DISPLAY);
		__TimeEvent = ID_TIMER_CLOSE_WINDOW;
		SetTimer(DialogHwnd, __TimeEvent, 5, NULL);
		break;
	}
	case ID_TIMER_POP_WINDOW:
	{
		if (Height <= WINDOW_HEIGHT)
		{
			Height += 3;
			MoveWindow(DialogHwnd, x, y - Height, WINDOW_WIDTH, Height, TRUE);
		}
		else
		{
			KillTimer(DialogHwnd, ID_TIMER_POP_WINDOW);
			__TimeEvent = ID_TIMER_DELAY_DISPLAY;
			SetTimer(DialogHwnd, __TimeEvent, 7000, NULL);
		}
		break;
	}
	}
}