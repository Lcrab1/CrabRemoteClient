#pragma once
#include "Manager.h"
#include <vector>
using namespace std;

typedef struct
{
    DWORD	FileSizeHigh;
    DWORD	FileSizeLow;
}FILE_SIZE;
class CFileManager :
    public CManager
{
public:
    CFileManager(CIocpClient* IocpClient);
    ~CFileManager();
    ULONG SendClientVolumeList();
    int SendClientFileList(PBYTE DirectoryFullPath);
    void HandleIo(PBYTE BufferData, ULONG_PTR BufferLength);
    VOID CreateReceivedFileInformation(LPBYTE BufferData);
    BOOL MakeSureDirectoryPathExists(char* DirectoryFullPath);
    VOID GetServerFileData();
    VOID WriteReceivedFileData(LPBYTE BufferData, ULONG BufferLength);
    VOID SetTransferMode(LPBYTE BufferData);

    char    m_FileFullPath[MAX_PATH];
    ULONG   m_TransferMode;
};

