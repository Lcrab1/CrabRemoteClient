#pragma once
#include "Manager.h"
#include"dllmain.h"
#include"InstantMessageManager.h"
#include"ProcessHelper.h"
#include"SystemHelper.h"
#include"ProcessManager.h"
#include"CmdManager.h"
class CKernelManager:public CManager
{
public:
	CKernelManager(CIocpClient* IocpClient);
	~CKernelManager();
	void HandleIo(PBYTE BufferData, ULONG_PTR BufferLength);
private:
	HANDLE m_ThreadHandles[0x1000];
	int    m_ThreadHandleCount;
};

DWORD WINAPI InstantMessageProcedure(LPVOID ParameterData);
DWORD WINAPI ProcessManagerProcedure(LPVOID ParameterData);
DWORD WINAPI CmdManagerProcedure(LPVOID ParameterData);