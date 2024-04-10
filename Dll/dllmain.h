#pragma once
#include <tchar.h>
#include <iostream>
#include "IocpClient.h"
#include "Login.h"
#include "KernelManager.h"
using namespace std;


extern char  __ServerAddress[MAX_PATH];
extern unsigned short __ConnectPort;
extern HINSTANCE __InstanceHandle;