#pragma once

#include <windows.h>

enum MYKEY {
	MHKEY_CLASSES_ROOT,
	MHKEY_CURRENT_USER,
	MHKEY_LOCAL_MACHINE,
	MHKEY_USERS,
	MHKEY_CURRENT_CONFIG
};


enum KEYVALUE {
	MREG_SZ,
	MREG_DWORD,
	MREG_BINARY,
	MREG_EXPAND_SZ
};



struct PACKET_HEADER {
	int   nameCount;          //名字个数
	DWORD nameSize;          //名字大小
	DWORD valueSize;          //值大小

};

class CRegister
{
public:
	CRegister(char IsToken);
	~CRegister();
	void SetPath(char* KeyPath);
	char* FindPath();
	char* FindKey();
private:
	HKEY m_KeyHandle;
	char m_KeyPath[MAX_PATH];
};

