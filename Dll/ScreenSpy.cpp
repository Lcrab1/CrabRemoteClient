#include "pch.h"
#include "ScreenSpy.h"



#define ALGORITHM_DIFF 1
CScreenSpy::CScreenSpy()
{
}
CScreenSpy::CScreenSpy(ULONG BitmapCount)
{
	m_Algorithm = ALGORITHM_DIFF;
	switch (BitmapCount)
	{
	case 16:
	case 32:
	{
		m_BitmapCount = BitmapCount;
		break;
	}
	default:
		m_BitmapCount = 16;
	}
	//�����Ļ�ֱ���
	m_FullMetricsWidth = GetSystemMetrics(SM_CXSCREEN);    
	m_FullMetricsHeight = GetSystemMetrics(SM_CYSCREEN);

	m_BitmapInfo = OnInitBitmapInfo(m_BitmapCount, m_FullMetricsWidth, m_FullMetricsHeight);  //����λͼ��Ϣ ����(��һ��)

	//�����Ļ���
	m_DesktopHwnd = GetDesktopWindow();

	//��һ������
	m_DesktopDCHandle = GetDC(m_DesktopHwnd);

	//������һ��������
	m_DesktopMemoryDCHandle = CreateCompatibleDC(m_DesktopDCHandle);
	
	//�����ʺϹ����õĹ���
	m_BitmapHandle = CreateDIBSection(m_DesktopDCHandle, m_BitmapInfo,
		DIB_RGB_COLORS, &m_BitmapData, NULL, NULL);
	
	//�ѹ��߷��뵽������
	SelectObject(m_DesktopMemoryDCHandle, m_BitmapHandle);


	m_DifficultMemoryDCHandle = CreateCompatibleDC(m_DesktopDCHandle);
	m_DifficultBitmapHandle = ::CreateDIBSection(m_DesktopDCHandle, m_BitmapInfo,
		DIB_RGB_COLORS, &m_DifficultBitmapData, NULL, NULL);
	::SelectObject(m_DifficultMemoryDCHandle, m_DifficultBitmapHandle);

	m_Offset = 0;
	m_BufferData = new BYTE[m_BitmapInfo->bmiHeader.biSizeImage * 2];

}

CScreenSpy::~CScreenSpy()
{

	//��͹���
	ReleaseDC(m_DesktopHwnd, m_DesktopDCHandle);   
	
	//���չ�����
	if(m_DesktopMemoryDCHandle != NULL)
	{
		DeleteDC(m_DesktopMemoryDCHandle);                

		//���ٹ���
		DeleteObject(m_BitmapHandle);
		if (m_BitmapData != NULL)
		{
			m_BitmapData = NULL;
		}

		m_DesktopMemoryDCHandle = NULL;

	}

    if(m_DifficultMemoryDCHandle != NULL)
	{
		DeleteDC(m_DifficultMemoryDCHandle);                

		DeleteObject(m_DifficultBitmapHandle);
		if (m_DifficultBitmapData != NULL)
		{
			m_DifficultBitmapData = NULL;
		}
	}

	if(m_BitmapInfo != NULL)   //������Ϣͷ
	{
		delete[] m_BitmapInfo;
		m_BitmapInfo = NULL;
	}

	if(m_BufferData)           //��������Ϣ
	{
		delete[] m_BufferData;
		m_BufferData = NULL;
	}

	m_Offset = 0;


}
LPBITMAPINFO CScreenSpy::OnInitBitmapInfo(ULONG BitmapCount,
	ULONG FullMetricsWidth, ULONG FullMetricsHeight)
{
	ULONG BufferLength = sizeof(BITMAPINFOHEADER);
	BITMAPINFO* BitmapInfo = (BITMAPINFO *) new BYTE[BufferLength];

	BITMAPINFOHEADER* BitmapInfoHeader = &(BitmapInfo->bmiHeader);



	BitmapInfoHeader->biSize = sizeof(BITMAPINFOHEADER);
	BitmapInfoHeader->biWidth =  FullMetricsWidth;    //1080
	BitmapInfoHeader->biHeight = FullMetricsHeight;   //1920
	BitmapInfoHeader->biPlanes = 1;
	BitmapInfoHeader->biBitCount = BitmapCount;       //16
	BitmapInfoHeader->biCompression = BI_RGB;
	BitmapInfoHeader->biXPelsPerMeter = 0;
	BitmapInfoHeader->biYPelsPerMeter = 0;
	BitmapInfoHeader->biClrUsed = 0;
	BitmapInfoHeader->biClrImportant = 0;
	BitmapInfoHeader->biSizeImage =
		((BitmapInfoHeader->biWidth * BitmapInfoHeader->biBitCount + 31) / 32) * 4 * BitmapInfoHeader->biHeight;

	return BitmapInfo;
}
ULONG CScreenSpy::GetBitmapInfoLength()
{
	return sizeof(BITMAPINFOHEADER);
}
LPBITMAPINFO CScreenSpy::GetBitmapInfo()
{
	return m_BitmapInfo;
}
LPVOID CScreenSpy::GetFirstScreenData()
{
	//���ڴ�ԭ�豸�и���λͼ��Ŀ���豸
	::BitBlt(m_DesktopMemoryDCHandle, 0, 0,
		m_FullMetricsWidth, m_FullMetricsHeight, m_DesktopDCHandle, 0, 0, SRCCOPY);

	return m_BitmapData;  //�ڴ�
}
ULONG CScreenSpy::GetFirstScreenLength()
{
	return m_BitmapInfo->bmiHeader.biSizeImage;
}
LPVOID CScreenSpy::GetNextScreenData(ULONG* BufferLength)
{
	if (BufferLength == NULL || m_BufferData == NULL)
	{
		return NULL;
	}

	m_Offset = 0;

	//д��ʹ���������㷨
	WriteScreenData((LPBYTE)&m_Algorithm, sizeof(m_Algorithm));

	//m_BufferData = [m_Algorithm]

	//��ÿͻ������λ��
	//д����λ��
	POINT	CursorPosition;
	GetCursorPos(&CursorPosition);
	WriteScreenData((LPBYTE)&CursorPosition, sizeof(POINT));
	//m_BufferData = [m_Algorithm][POINT]
	//��ÿͻ��˹�������
	//д�뵱ǰ�������
	BYTE	CursorTypeIndex = -1; //���дһ��
	WriteScreenData(&CursorTypeIndex, sizeof(BYTE));
	//m_BufferData = [1][POINT][CursorTypeIndex][][][][][][]
	
	// ����Ƚ��㷨
	if (m_Algorithm == ALGORITHM_DIFF)
	{
		// �ֶ�ɨ��ȫ��Ļ  ���µ�λͼ���뵽m_hDiffMemDC��
		ScanScreenData(m_DifficultMemoryDCHandle, m_DesktopDCHandle, m_BitmapInfo->bmiHeader.biWidth,
			m_BitmapInfo->bmiHeader.biHeight);

		//����Bitmap���бȽ������һ��
		*BufferLength = m_Offset +
			CompareScreenData((LPBYTE)m_DifficultBitmapData, (LPBYTE)m_BitmapData,
				m_BufferData + m_Offset, m_BitmapInfo->bmiHeader.biSizeImage);

		//v1               v2       
		//WorlWorldAA      WorldHelloAA
		//                                                 Offset          *v11         *v22
		//m_BufferData = [m_Algorithm][POINT][CursorTypeIndex][λ��(DWORD)]    [(Count)]    [Wo]
		return m_BufferData;
	}
	return NULL;
}
VOID CScreenSpy::WriteScreenData(LPBYTE	BufferData, ULONG BufferLength)
{
	memcpy(m_BufferData + m_Offset, BufferData, BufferLength);
	m_Offset += BufferLength;
}
VOID CScreenSpy::ScanScreenData(HDC DestinationDCHandle, HDC SourceDCHandle, ULONG Width, ULONG Height)
{
	ULONG	JumpLine = 50;
	ULONG	JumpSleep = JumpLine / 10;

	for (int i = 0, ToJump = 0; i < Height; i += ToJump)
	{
		ULONG  v1 = Height - i;
		if (v1 > JumpLine)
		{
			//ÿ�ΰ���50����λ����ɨ��
			ToJump = JumpLine;  
		}		
		else
		{
			//���һ��
			ToJump = v1;
		}
			
		BitBlt(DestinationDCHandle, 0, i, Width, ToJump, SourceDCHandle, 0, i, SRCCOPY);
		Sleep(JumpSleep);
	}
}
ULONG CScreenSpy::CompareScreenData(LPBYTE NextScreenData, LPBYTE FirstScreenData,
	LPBYTE BufferData, DWORD ScreenLength)
{
	//Windows�涨һ��ɨ������ռ���ֽ���������4�ı���, ������DWORD�Ƚ�
	LPDWORD	v1, v2;
	v1 = (LPDWORD)FirstScreenData;
	v2 = (LPDWORD)NextScreenData;

	// ƫ�Ƶ�ƫ�ƣ���ͬ���ȵ�ƫ��
	ULONG Offset = 0, v11 = 0, v22 = 0;
	ULONG Count = 0;   
		
	for (int i = 0; i < ScreenLength; i += 4, v1++, v2++)   
	{
		if (*v1 == *v2)
		{
			//��֡����һ��
			continue;
		}	
		//BufferData + m_Offset         4
		//BufferData[1][Point][Type]   [2][][x]

		//�������ݲ�һ��д�뵽�ڴ�
		*(LPDWORD)(BufferData + Offset) = i; //λ��
		// ��¼���ݴ�С�Ĵ��λ��
		v11 = Offset + sizeof(DWORD);  //4
		v22 = v11 + sizeof(DWORD);     //8
		Count = 0; 
		//���ݼ���������

		//����ǰһ֡����
		*v1 = *v2;
		*(LPDWORD)(BufferData + v22 + Count) = *v2;
		//[]
		//[x][x]

		//BufferData + m_Offset         4
	    //BufferData[1][Point][Type]   [2][2][x][x]
		Count += 4;
		i += 4, v1++, v2++;

		//Hex xoWorld   v1
		//Hex xoWorld   v2

		for (int j = i; j < ScreenLength; j += 4, i += 4, v1++, v2++)
		{
			if (*v1 == *v2)
				break;

			// ����Dest�е�����
			*v1 = *v2;
			*(LPDWORD)(BufferData + v22 + Count) = *v2;
			Count += 4;
		}

		//Hex xoWorld   v1
		//Hex xoWorld   v2
		// д�����ݳ���
		*(LPDWORD)(BufferData + v11) = Count;
		Offset = v22 + Count;
	}

	// nOffsetOffset ����д����ܴ�С
	return Offset;
}

