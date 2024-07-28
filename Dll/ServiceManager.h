#pragma once
#include "Manager.h"
#include"ProcessHelper.h"
#include"Common.h"
class CServiceManager :
    public CManager
{
public:
    CServiceManager(CIocpClient* IocpClient);
    ~CServiceManager();
    void HandleIo(PBYTE BufferData, ULONG_PTR BufferLength);
    VOID SendClientServiceList();
    LPBYTE GetClientServiceList();

    SC_HANDLE m_ServiceHandle;
    void ConfigClientService(PBYTE BufferData, ULONG BufferLength);
};

