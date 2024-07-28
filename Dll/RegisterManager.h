#pragma once
#pragma once
#include "Manager.h"
#include <vector>
using namespace std;
class CRegisterManager :
    public CManager
{
public:
    CRegisterManager(CIocpClient* IocpClient);
    ~CRegisterManager();
    BOOL SendClientProcessList();
    void HandleIo(PBYTE BufferData, ULONG_PTR BufferLength);
    VOID FindRegisterData(char IsToken, char* KeyPath);

    void AddPath(char* BufferData);
    void AddKey(char* BufferData);

};


