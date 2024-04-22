#pragma once
#include<iostream>
#include<Windows.h>
#include<list>
class CVMMap
{
public:
	CVMMap();
	~CVMMap();
	void GetSystemInfo();

	VOID GetMemoryStatus();
	BOOL GetMemoryBasicInfo(HANDLE ProcessIdentity);
	BOOL SeEnableSeDebugPrivilege(HANDLE ProcessHandle, BOOL IsEnable);
	HANDLE SeOpenProcess(DWORD DesiredAccess, BOOL IsInheritHandle, HANDLE ProcessIdentity);
	BOOL SeCloseHandle(HANDLE HandleValue);
public:
	SYSTEM_INFO SystemInfo = { 0 };
	MEMORYSTATUS MemoryStatus = { sizeof(MEMORYSTATUS) };
	std::list<MEMORY_BASIC_INFORMATION> MemoryBasicInfoList;
	BOOL   m_EnableDebugPrivilege = TRUE;
	HANDLE m_ProcessHandle = GetCurrentProcess();

};

