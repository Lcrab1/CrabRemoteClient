#pragma once

#include <windows.h>
#include <iostream>
using namespace std;
class CScreenSpy
{
public:
	CScreenSpy();
	CScreenSpy(ULONG BitmapCount);
	~CScreenSpy();
	LPBITMAPINFO OnInitBitmapInfo(ULONG BitmapCount,
		ULONG FullMetricsWidth, ULONG FullMetricsHeight);
	//位图信息
	ULONG GetBitmapInfoLength();
	LPBITMAPINFO GetBitmapInfo();
	//抓图
	LPVOID GetFirstScreenData();
	ULONG GetFirstScreenLength();
	
	
	
	LPVOID GetNextScreenData(ULONG* BufferLength);
	//构建发送的数据包
	VOID WriteScreenData(LPBYTE	BufferData, ULONG BufferLength);
	//分段拷贝位图数据
	VOID ScanScreenData(HDC DestinationDCHandle, HDC SourceDCHandle, ULONG Width, ULONG Height);
	//比较两帧数据
	ULONG CompareScreenData(LPBYTE NextScreenData, LPBYTE FirstScreenData,
		LPBYTE BufferData, DWORD ScreenScreenLength);
public:
	BYTE   m_Algorithm;                   //算法(抓图)
	ULONG  m_BitmapCount;             
	int    m_FullMetricsWidth;
	int    m_FullMetricsHeight;
	LPBITMAPINFO     m_BitmapInfo;
	HWND   m_DesktopHwnd;                 //桌面句柄
	HDC    m_DesktopDCHandle;             //工人
	HDC    m_DesktopMemoryDCHandle;       //工具箱
	HBITMAP	m_BitmapHandle;               //工具
	PVOID   m_BitmapData;

	HDC     m_DifficultMemoryDCHandle;
	HBITMAP	m_DifficultBitmapHandle;
	PVOID   m_DifficultBitmapData;

	//发送到服务器中的数据包内存
	BYTE*   m_BufferData;
	ULONG   m_Offset;
};

