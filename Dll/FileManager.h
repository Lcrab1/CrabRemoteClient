#pragma once
#include "Manager.h"
#include <vector>
using namespace std;
class CFileManager :
    public CManager
{
public:
    CFileManager(CIocpClient* IocpClient);
    ~CFileManager();
    ULONG SendClientVolumeList();
    int SendClientFileList(PBYTE DirectoryFullPath);
    void HandleIo(PBYTE BufferData, ULONG_PTR BufferLength);
};

