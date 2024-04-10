#include"ClientProject.h"

struct _CONNECT_INFORMATION_
{
	DWORD    CheckFlag;   //随便定义数字
	char     SeverAddress[20];    //IP 四个单字
	USHORT   ConnectPort;         //王艳萍
}__ConnectInfo = { 0x87654321,"127.0.0.1",2356 };    //echo  回声

//客户端向服务端链接信息
void _tmain()
{
	_tprintf(_T("ServerAddress:%s\r\n"), __ConnectInfo.SeverAddress);
	_tprintf(_T("ConnectPort:%d\r\n"), __ConnectInfo.ConnectPort);
	//MessageBox(NULL, "nothing", NULL, 0);
	//加载Dll.dll模块
	HMODULE  ModuleHandle = (HMODULE)LoadLibrary(_T("Dll.dll"));   //加载自己写的Module
	if (ModuleHandle == NULL)
	{
		return;
	}
	//获取一个Dll模块中的一个导出函数
	LPFN_CLIENTRUN ClientRun =
		(LPFN_CLIENTRUN)GetProcAddress(ModuleHandle, "ClientRun");


	//没有获取到Dll.dll中的导出函数
	if (ClientRun == NULL)
	{
		FreeLibrary(ModuleHandle);   //释放模块 退出
		return;
	}

	else
	{
		ClientRun(__ConnectInfo.SeverAddress, __ConnectInfo.ConnectPort);
	}
	


	_tprintf(_T("Input AnyKey To Exit\r\n"));
	_gettchar();
	FreeLibrary(ModuleHandle);   //释放模块

}