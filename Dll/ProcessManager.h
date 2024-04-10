#pragma once
#include "Manager.h"
#include"Common.h"
#include"ProcessHelper.h"
#include"MemoryHelper.h"
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
public:
    HANDLE                  m_CurrentProcessID;
    std::vector<size_t>*    m_Address;
    int                     m_ElementCount;
    HANDLE                  m_TargetHandle;
    BYTE                     m_ScanRelpy;
};

