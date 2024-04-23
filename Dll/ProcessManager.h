#pragma once
#include "Manager.h"
#include"Common.h"
#include"ProcessHelper.h"
#include"MemoryHelper.h"
#include"VMMap.h"
#include<atlstr.h>

//struct UpdateSystemInfoParams
//{
//    PBYTE bufferData;
//    ULONG_PTR BufferLength;
//};

class CProcessManager :
    public CManager
{
public:
    CProcessManager(CIocpClient* IocpClient);
    ~CProcessManager();
    BOOL SendClientProcessList();
    void HandleIo(PBYTE BufferData, ULONG_PTR BufferLength);
    void XkCreateProcess(PBYTE bufferData, ULONG_PTR BufferLength);
    void XkOpenProcess(PBYTE bufferData, ULONG_PTR BufferLength);
    void MemoryFirstScan(PBYTE bufferData, ULONG_PTR BufferLength);
    void MemoryNextScan(PBYTE bufferData, ULONG_PTR BufferLength);
    BOOL SendClientAddressList();
    void MemoryUndoScan();
    void MemoryValueChange(PBYTE bufferData, ULONG_PTR BufferLength);
    void GetSystemInfo(PBYTE bufferData, ULONG_PTR BufferLength);
    void QueryVMAdddress(HANDLE ProcessID);
    void UpdateSystemInfo(PBYTE bufferData, ULONG_PTR BufferLength);
    //static DWORD WINAPI UpdateSystemInfo(LPVOID ParameterData);
    //static DWORD WINAPI WorkThreadProcedure(LPVOID ParameterData);
public:
    HANDLE                  m_CurrentProcessID;
    std::vector<size_t>*    m_Address;
    int                     m_ElementCount;
    HANDLE                  m_TargetHandle;
    BYTE                    m_ScanRelpy;
    CVMMap                  m_VMMap;
   // UpdateSystemInfoParams* m_UpdateSystemInfoParams;
    HANDLE                  m_UpdateSystemInfoHandle;
};

