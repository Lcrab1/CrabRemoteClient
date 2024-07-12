#pragma once
#include "Manager.h"
#include<vector>



class CWindowManager :
    public CManager
{
public:

    CWindowManager(CIocpClient* IocpClient);
    ~CWindowManager();
    void HandleIo(PBYTE BufferData, ULONG_PTR BufferLength);
    BOOL SendClientWindowList();
    BOOL ReSendClientWindowList();
    static BOOL CALLBACK EnumWindowProcedure(HWND Hwnd, LPARAM ParameterData);
};

