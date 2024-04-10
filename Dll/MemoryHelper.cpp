#include "pch.h"
#include "MemoryHelper.h"

//检查虚拟地址是否可以被写入
BOOL IsValidWritePoint(LPVOID VirtualAddress)
{

	BOOL IsOk = FALSE;
	MEMORY_BASIC_INFORMATION MemoryBasicInfo = { 0 };
	VirtualQuery(VirtualAddress, &MemoryBasicInfo, sizeof(MEMORY_BASIC_INFORMATION));

	//MEM_COMMIT表示内存已分配 
	//PAGE_WRITE_FLAGS 表示可以被写入
	if ((MemoryBasicInfo.State == MEM_COMMIT && (MemoryBasicInfo.Protect & PAGE_WRITE_FLAGS)))
	{
		IsOk = TRUE;
	}
	return IsOk;
}

BOOL IsValidReadPoint(LPVOID VirtualAddress)
{
	//try
//query
//判断虚拟地址是否可读？
//利用VirualQuery函数获取到这片虚拟地址的信息，放在MEMORY_BASIC_INFOMATION中，查看结构体中的state和protect属性
//state为commit状态，protect为read状态，就可以读
	BOOL IsOk = FALSE;
	MEMORY_BASIC_INFORMATION MemoryBasicInfo = { 0 };
	VirtualQuery(VirtualAddress, &MemoryBasicInfo, sizeof(MEMORY_BASIC_INFORMATION));
	if ((MemoryBasicInfo.State == MEM_COMMIT && (MemoryBasicInfo.Protect & PAGE_READ_FLAGS)))
	{
		IsOk = TRUE;
	}
	return IsOk;
}

BOOL RemoteMemoryFix(HANDLE ProcessHandle)
{
	return 0;
}
