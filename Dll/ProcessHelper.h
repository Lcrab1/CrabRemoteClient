#pragma once
#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <vector>
#include <TlHelp32.h>
#include <Psapi.h>
#pragma comment(lib,"Psapi.lib")
#include <shellapi.h>
#include"MemoryHelper.h"
using namespace std;

#pragma pack(1)
typedef struct PROCESS_INFORMATION_ITEM
{
	HANDLE ProcessIdentity;
	TCHAR   ProcessImageName[MAX_PATH];
	TCHAR   ProcessFullPath[MAX_PATH];
	TCHAR   isWow64Process[20];
}PROCESS_INFORMATION_ITEM, * PPROCESS_INFORMATION_ITEM;


int EnableSeDebugPrivilege(HANDLE ProcessHandle, BOOL IsEnable, LPCTSTR RequireLevel);
BOOL EnableDebugPrivilege(IN const TCHAR* PriviledgeName, BOOL IsEnable);
BOOL XkIsWow64Process(HANDLE ProcessHandle, BOOL* isWow64Process);
BOOL EnumProcessByToolHelp32(vector<PROCESS_INFORMATION_ITEM>& ProcessInfo);
VOID KillProcess(LPBYTE BufferData, UINT BufferLength);
BOOL OpenProcessByProcessID(HANDLE ProcessIdentity, HANDLE* ProcessHandle);