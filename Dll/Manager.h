#pragma once
#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include "IocpClient.h"

using namespace std;

//抽象类  -- --- --- --- ---

class CIocpClient;
class CManager
{
public:
	CManager(CIocpClient* IocpClient);
	~CManager();
	virtual void HandleIo(PBYTE BufferData, ULONG_PTR BufferLength)
	{

	}
	VOID WaitingForDialogOpen()
	{
		WaitForSingleObject(m_EventOpenDialogHandle, INFINITE);
		//必须的Sleep,因为远程窗口从InitDialog中发送COMMAND_NEXT到显示还要一段时间
		Sleep(150);
	}
	VOID NotifyDialogIsOpen()
	{
		SetEvent(m_EventOpenDialogHandle);
	}

public:
	CIocpClient* m_IocpClient;   //通信类对象指针
	HANDLE m_EventOpenDialogHandle;
};