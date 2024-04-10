#pragma once
#include <iostream>
#include<Windows.h>
#include<tchar.h>
#include<TlHelp32.h>
#include<ntverp.h>
using namespace std;
#define PAGE_SIZE 0x1000

#define PAGE_READ_FLAGS \
    (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)
#define PAGE_WRITE_FLAGS \
        (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)

#define STATUS_SUCCESS                  (NTSTATUS)0x00000000

#ifdef _WIN64
#define  RING3_LIMITED 0x00007FFFFFFEFFFF   
#else
#define  RING3_LIMITED 0x7FFEFFFF
#endif // _WIN64
BOOL IsValidWritePoint(LPVOID VirtualAddress);
BOOL IsValidReadPoint(LPVOID VirtualAddress);
BOOL RemoteMemoryFix(HANDLE ProcessHandle);