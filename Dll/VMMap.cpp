#include "pch.h"
#include "VMMap.h"

CVMMap::CVMMap()
{
}

CVMMap::~CVMMap()
{
	if (m_ProcessHandle != INVALID_HANDLE_VALUE)
	{

		SeCloseHandle(m_ProcessHandle);
		m_ProcessHandle = INVALID_HANDLE_VALUE;
	}
}

void CVMMap::GetSystemInfo()
{
	::GetSystemInfo(&SystemInfo);
}

VOID CVMMap::GetMemoryStatus()
{
	GlobalMemoryStatus(&this->MemoryStatus);
}

BOOL CVMMap::GetMemoryBasicInfo(HANDLE ProcessIdentity)
{
	// 清空虚拟内存信息链表
	this->MemoryBasicInfoList.clear();   //

	//穿入的进程Identity不是自己
	if ((DWORD)ProcessIdentity != GetCurrentProcessId())
	{
		// 打开目标进程
		m_ProcessHandle = SeOpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, ProcessIdentity);
		if (m_ProcessHandle == NULL)
		{
			return FALSE;
		}
	}

	MEMORY_BASIC_INFORMATION	MemoryBasicInfo = { 0 };
	DWORD MemoryBasicInfoLength = sizeof(MEMORY_BASIC_INFORMATION);


	ULONG_PTR StartAddress = (ULONG_PTR)this->SystemInfo.lpMinimumApplicationAddress;
	while (StartAddress < (ULONG_PTR)this->SystemInfo.lpMaximumApplicationAddress)
	{
		// 查询指定进程的指定地址的状态信息
		VirtualQueryEx(m_ProcessHandle, (LPVOID)StartAddress, &MemoryBasicInfo, MemoryBasicInfoLength);
		// 把状态信息添加到链表
		this->MemoryBasicInfoList.push_back(MemoryBasicInfo);
		// 定位到下一个区域
		StartAddress = (ULONG_PTR)MemoryBasicInfo.BaseAddress + MemoryBasicInfo.RegionSize;
	}

	if (m_ProcessHandle != INVALID_HANDLE_VALUE)
	{

		SeCloseHandle(m_ProcessHandle);
		m_ProcessHandle = INVALID_HANDLE_VALUE;
	}
	return (this->MemoryBasicInfoList.size() > 0);
}

BOOL CVMMap::SeEnableSeDebugPrivilege(HANDLE ProcessHandle, BOOL IsEnable)
{
	DWORD  LastError;
	HANDLE TokenHandle = 0;

	if (!OpenProcessToken(ProcessHandle, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &TokenHandle))
	{
		LastError = GetLastError();
		if (TokenHandle)
			CloseHandle(TokenHandle);
		return LastError;
	}
	TOKEN_PRIVILEGES TokenPrivileges;
	memset(&TokenPrivileges, 0, sizeof(TOKEN_PRIVILEGES));
	LUID v1;
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &v1))
	{
		LastError = GetLastError();
		CloseHandle(TokenHandle);
		return LastError;
	}
	TokenPrivileges.PrivilegeCount = 1;
	TokenPrivileges.Privileges[0].Luid = v1;
	if (IsEnable)
		TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		TokenPrivileges.Privileges[0].Attributes = 0;
	AdjustTokenPrivileges(TokenHandle, FALSE, &TokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
	LastError = GetLastError();
	CloseHandle(TokenHandle);
	return LastError;
}

HANDLE CVMMap::SeOpenProcess(DWORD DesiredAccess, BOOL IsInheritHandle, HANDLE ProcessIdentity)
{
	if (m_EnableDebugPrivilege)
	{
		SeEnableSeDebugPrivilege(GetCurrentProcess(), TRUE);
	}
	HANDLE ProcessHandle = OpenProcess(DesiredAccess, IsInheritHandle, (DWORD)ProcessIdentity);

	DWORD LastError = GetLastError();
	if (m_EnableDebugPrivilege)
	{
		SeEnableSeDebugPrivilege(GetCurrentProcess(), FALSE);
	}
	SetLastError(LastError);
	return ProcessHandle;
}

BOOL CVMMap::SeCloseHandle(HANDLE HandleValue)
{
	DWORD HandleFlags;
	if (GetHandleInformation(HandleValue, &HandleFlags)
		&& (HandleFlags & HANDLE_FLAG_PROTECT_FROM_CLOSE) != HANDLE_FLAG_PROTECT_FROM_CLOSE)
		return !!CloseHandle(HandleValue);
	return FALSE;
}
