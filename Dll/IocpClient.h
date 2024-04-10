#pragma once

#include<winsock2.h>
#include<windows.h>
#include<iostream>
#include"CArray.h"
#include"zconf.h"
#include"zlib.h"
#include"Manager.h"

using namespace std;
#pragma comment(lib,"WS2_32.lib")
#define PACKET_LENGTH 0x2000
#define PACKET_HEADER_LENGTH 13
#define PACKET_FLAG_LENGTH 5
#define MAX_SEND_BUFFER 0x2000

class CIocpClient
{
public:
	CIocpClient();
	~CIocpClient();
	BOOL ConnectServer(char* ServerAddress, unsigned short ConnectPort);
	static DWORD WINAPI WorkThreadProcedure(LPVOID ParameterData);
	BOOL IsReceiving()
	{
		return m_IsReceiving;
	}
	VOID SetManagerObject(class CManager* Manager)
	{
		//面向对象(多态(抽象类指针指向实例对象地址))
		m_Manager = Manager;
	}
	VOID WaitingForEvent()
	{
		//避免对象销毁
		WaitForSingleObject(m_EventHandle, INFINITE);
	}
	VOID Disconnect();
	VOID OnReceiving(char* BufferData, ULONG BufferLength);
	int OnSending(char* BufferData, ULONG BufferLength);
	BOOL SendWithSplit(char* BufferData, ULONG BufferLength, ULONG SplitLength);

private:
	BOOL m_IsReceiving = TRUE;
	CArray_  m_SendBufferDataCompressed;
	CArray_  m_ReceivedBufferDataCompressed;
	CArray_  m_ReceivedBufferDataDecompressed;
	char m_PacketHeaderFlag[PACKET_FLAG_LENGTH];

	//多态性
	CManager* m_Manager;
public:
	SOCKET m_ClientSocket;
	HANDLE m_WorkThreadHandle;
	HANDLE m_EventHandle;
};

	