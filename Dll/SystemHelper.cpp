#include "pch.h"
#include "SystemHelper.h"

typedef enum _SHUTDOWN_ACTION
{
	ShutdownNoReboot,
	ShutdownReboot,
	ShutdownPowerOff
} SHUTDOWN_ACTION;

void ShutdownSystem()
{
	//获得ntdll模块的函数
	HMODULE NtdllModuleBase = LoadLibrary(_T("Ntdll.DLL"));
	if (NtdllModuleBase == NULL)
	{
		return;
	}
	typedef int(_stdcall* LPFN_ZWSHUTDOWNSYSTEM)(int);
	LPFN_ZWSHUTDOWNSYSTEM  ZwShutdownSystem = NULL;
	ZwShutdownSystem = (LPFN_ZWSHUTDOWNSYSTEM)GetProcAddress(NtdllModuleBase, "ZwShutdownSystem");

	if (ZwShutdownSystem == NULL)
	{
		goto Exit;
	}
	//关机
	//ZwShutdownSystem(ShutdownPowerOff);
Exit:
	if (NtdllModuleBase != NULL)
	{
		FreeLibrary(NtdllModuleBase);
 		NtdllModuleBase = NULL;
	}
}