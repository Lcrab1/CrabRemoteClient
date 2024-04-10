#include "pch.h"
#include "CArray.h"
#define U_PAGE_ALIGNMENT   3
#define F_PAGE_ALIGNMENT 3.0

CArray_::CArray_()
{
	m_MaximumLength = 0;
	m_BufferData = m_CheckPosition = NULL;
	InitializeCriticalSection(&m_CriticalSection);   //临界区 关键段 线程同步
}

CArray_::~CArray_()
{
	if (m_BufferData)
	{
		VirtualFree(m_BufferData, 0, MEM_RELEASE);   //进程虚拟地址空间
		m_BufferData = NULL;
	}

	DeleteCriticalSection(&m_CriticalSection);
	m_BufferData = m_CheckPosition = NULL;
	m_MaximumLength = 0;
}

BOOL CArray_::WriteArray(PUINT8 BufferData, ULONG_PTR BufferLength)
{
	EnterCriticalSection(&m_CriticalSection);

	//Array[][][][][][][][][][][][][] HelloWorld    xx

	// 
	//[5][4][3][2][1] 1 1 1 1   [][] [] [] 

	//[][][][][][][]  [] [] [] [] 

	if (ReallocateArray(BufferLength + GetArrayLength()) == (ULONG_PTR)-1)   //内存空间不足
	{
		LeaveCriticalSection(&m_CriticalSection);  //申请失败
		return FALSE;
	}

	CopyMemory(m_CheckPosition, BufferData, BufferLength);

	m_CheckPosition += BufferLength;
	LeaveCriticalSection(&m_CriticalSection);
	return TRUE;
}

//重新分配数组大小，并将旧数据拷贝进去
ULONG_PTR CArray_::ReallocateArray(ULONG_PTR BufferLength)
{
	//一般传入的bufferlength是当前数据+新数据的长度总和
	if (BufferLength < GetArrayMaximumLength())
		return 0;

	
	//向上取边界 3的倍数   11/3.0 = 
	ULONG_PTR  v7 = (ULONG_PTR)ceil(BufferLength / F_PAGE_ALIGNMENT) * U_PAGE_ALIGNMENT;
	PUINT8  v5 = (PUINT8)VirtualAlloc(NULL, v7, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (v5 == NULL)
	{
		return -1;
	}
	//原先的有效数据长度  
	ULONG_PTR v3 = GetArrayLength();
	//拷贝原先数据到新的申请的内存中
	CopyMemory(v5, m_BufferData, v3);

	//释放原先内存
	if (m_BufferData)
	{
		VirtualFree(m_BufferData, 0, MEM_RELEASE);
	}
	m_BufferData = v5;
	m_CheckPosition = m_BufferData + v3;   //空闲内存处

	m_MaximumLength = v7;

	return m_MaximumLength;
}


ULONG_PTR CArray_::GetArrayMaximumLength()
{
	return m_MaximumLength;
}


ULONG_PTR CArray_::GetArrayLength()
{
	if (m_BufferData == NULL)
		return 0;

	return (ULONG_PTR)m_CheckPosition - (ULONG_PTR)m_BufferData;
}

PUINT8 CArray_::GetArray(ULONG_PTR Position)
{
	if (m_BufferData == NULL)
	{
		return NULL;
	}

	if (Position >= GetArrayLength())
	{
		return NULL;
	}
	return m_BufferData + Position;
}

VOID CArray_::ClearArray()
{
	EnterCriticalSection(&m_CriticalSection);
	m_CheckPosition = m_BufferData;
	//只保留1024内存
	DeallocateArray(1024);  //回收内存
	LeaveCriticalSection(&m_CriticalSection);
}

ULONG_PTR CArray_::DeallocateArray(ULONG_PTR BufferLength)
{
	//4096内存空间
	//大于1024的有效数据
	if (BufferLength < GetArrayLength())  //HelloWorld[][][][][][][][][][][][][][][][][][][][][][][][][][][]
		return 0;

	//有效数据小于1024  只保留1024的内存空间
	ULONG_PTR v7 = (ULONG_PTR)ceil(BufferLength / F_PAGE_ALIGNMENT) * U_PAGE_ALIGNMENT;

	if (GetArrayMaximumLength() <= v7)
	{
		return 0;
	}
	//重新申请新的1024的内存
	PUINT8 v5 = (PUINT8)VirtualAlloc(NULL, v7, MEM_COMMIT, PAGE_READWRITE);

	ULONG_PTR v3 = GetArrayLength();  //算原先内存的有效长度   车库的车
	CopyMemory(v5, m_BufferData, v3);

	VirtualFree(m_BufferData, 0, MEM_RELEASE);

	m_BufferData = v5;
	m_CheckPosition = m_BufferData + v3;
	m_MaximumLength = v7;
	return m_MaximumLength;
}

ULONG_PTR CArray_::ReadArray(PUINT8 BufferData, ULONG_PTR BufferLength)
{
	EnterCriticalSection(&m_CriticalSection);
	if (BufferLength > GetArrayMaximumLength())  //内存空间长   有多少个车库
	{
		LeaveCriticalSection(&m_CriticalSection);
		return 0;
	}
	if (BufferLength > GetArrayLength())   //400车库  300车   350车 
	{
		BufferLength = GetArrayLength();   //300车
	}

	if (BufferLength)
	{
		CopyMemory(BufferData, m_BufferData, BufferLength);//将Arry数据拷贝到参数中
		//数组前移覆盖
		MoveMemory(m_BufferData, m_BufferData + BufferLength, GetArrayMaximumLength() - BufferLength);
		m_CheckPosition -= BufferLength;
	}
	DeallocateArray(GetArrayLength());//回收车库 
	LeaveCriticalSection(&m_CriticalSection);
	return BufferLength;
}

ULONG_PTR CArray_::RemoveArray(ULONG_PTR BufferLength)
{
	EnterCriticalSection(&m_CriticalSection);
	if (BufferLength > GetArrayMaximumLength())
	{
		return 0;
	}
	if (BufferLength > GetArrayLength())
	{
		BufferLength = GetArrayLength();
	}
	if (BufferLength)
	{
		//将车库里车前移覆盖
		MoveMemory(m_BufferData, m_BufferData + BufferLength, GetArrayMaximumLength() - BufferLength);   //数组前移  [Shinexxxx??]
		m_CheckPosition -= BufferLength;
	}
	DeallocateArray(GetArrayLength());
	LeaveCriticalSection(&m_CriticalSection);
	return BufferLength;
}
