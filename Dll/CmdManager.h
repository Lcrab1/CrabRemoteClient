#pragma once
#include "Manager.h"
#include"ProcessHelper.h"
#include"Common.h"
#include <vector>
using namespace std;
class CCmdManager
    :public CManager
{
public:
    CCmdManager(CIocpClient* IocpClient);
    ~CCmdManager();
    void HandleIo(PBYTE BufferData, ULONG_PTR BufferLength);
    static  DWORD WINAPI  ReceiveProcedure(LPVOID ParameterData);    //从Cmd获取数据

public:
    HANDLE  m_ClientReadHandle;
    HANDLE  m_ClientWriteHandle;
    HANDLE  m_CmdReadHandle;
    HANDLE  m_CmdWriteHandle;
    HANDLE  m_ThreadHandle;
    HANDLE  m_CmdProcessHandle;
    HANDLE  m_CmdThreadHandle;
    BOOL    m_IsLoop;


};

