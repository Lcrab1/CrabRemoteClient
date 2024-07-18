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
	//获得屏幕分辨率
	m_FullMetricsWidth = GetSystemMetrics(SM_CXSCREEN);    
	m_FullMetricsHeight = GetSystemMetrics(SM_CYSCREEN);

	m_BitmapInfo = OnInitBitmapInfo(m_BitmapCount, m_FullMetricsWidth, m_FullMetricsHeight);  //构建位图信息 发送(第一波)

	//获得屏幕句柄
	m_DesktopHwnd = GetDesktopWindow();

	//请一个工人
	m_DesktopDCHandle = GetDC(m_DesktopHwnd);

	//给工人一个工具箱
	m_DesktopMemoryDCHandle = CreateCompatibleDC(m_DesktopDCHandle);
	
	//创建适合工人用的工具
	m_BitmapHandle = CreateDIBSection(m_DesktopDCHandle, m_BitmapInfo,
		DIB_RGB_COLORS, &m_BitmapData, NULL, NULL);
	
	//把工具放入到工具箱
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

	//解雇工人
	ReleaseDC(m_DesktopHwnd, m_DesktopDCHandle);   
	
	//回收工具箱
	if(m_DesktopMemoryDCHandle != NULL)
	{
		DeleteDC(m_DesktopMemoryDCHandle);                

		//销毁工具
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

	if(m_BitmapInfo != NULL)   //发出信息头
	{
		delete[] m_BitmapInfo;
		m_BitmapInfo = NULL;
	}

	if(m_BufferData)           //发出的信息
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
	//用于从原设备中复制位图到目标设备
	::BitBlt(m_DesktopMemoryDCHandle, 0, 0,
		m_FullMetricsWidth, m_FullMetricsHeight, m_DesktopDCHandle, 0, 0, SRCCOPY);

	return m_BitmapData;  //内存
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

	//写入使用了哪种算法
	WriteScreenData((LPBYTE)&m_Algorithm, sizeof(m_Algorithm));

	//m_BufferData = [m_Algorithm]

	//获得客户端鼠标位置
	//写入光标位置
	POINT	CursorPosition;
	GetCursorPos(&CursorPosition);
	WriteScreenData((LPBYTE)&CursorPosition, sizeof(POINT));
	//m_BufferData = [m_Algorithm][POINT]
	//获得客户端光标的样子
	//写入当前光标类型
	BYTE	CursorTypeIndex = -1; //随便写一个
	WriteScreenData(&CursorTypeIndex, sizeof(BYTE));
	//m_BufferData = [1][POINT][CursorTypeIndex][][][][][][]
	
	// 差异比较算法
	if (m_Algorithm == ALGORITHM_DIFF)
	{
		// 分段扫描全屏幕  将新的位图放入到m_hDiffMemDC中
		ScanScreenData(m_DifficultMemoryDCHandle, m_DesktopDCHandle, m_BitmapInfo->bmiHeader.biWidth,
			m_BitmapInfo->bmiHeader.biHeight);

		//两个Bitmap进行比较如果不一样
		*BufferLength = m_Offset +
			CompareScreenData((LPBYTE)m_DifficultBitmapData, (LPBYTE)m_BitmapData,
				m_BufferData + m_Offset, m_BitmapInfo->bmiHeader.biSizeImage);

		//v1               v2       
		//WorlWorldAA      WorldHelloAA
		//                                                 Offset          *v11         *v22
		//m_BufferData = [m_Algorithm][POINT][CursorTypeIndex][位置(DWORD)]    [(Count)]    [Wo]
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
			//每次按照50个单位进行扫描
			ToJump = JumpLine;  
		}		
		else
		{
			//最后一次
			ToJump = v1;
		}
			
		BitBlt(DestinationDCHandle, 0, i, Width, ToJump, SourceDCHandle, 0, i, SRCCOPY);
		Sleep(JumpSleep);
	}
}
ULONG CScreenSpy::CompareScreenData(LPBYTE NextScreenData, LPBYTE FirstScreenData,
	LPBYTE BufferData, DWORD ScreenLength)
{
	//Windows规定一个扫描行所占的字节数必须是4的倍数, 所以用DWORD比较
	LPDWORD	v1, v2;
	v1 = (LPDWORD)FirstScreenData;
	v2 = (LPDWORD)NextScreenData;

	// 偏移的偏移，不同长度的偏移
	ULONG Offset = 0, v11 = 0, v22 = 0;
	ULONG Count = 0;   
		
	for (int i = 0; i < ScreenLength; i += 4, v1++, v2++)   
	{
		if (*v1 == *v2)
		{
			//两帧数据一样
			continue;
		}	
		//BufferData + m_Offset         4
		//BufferData[1][Point][Type]   [2][][x]

		//发现数据不一样写入到内存
		*(LPDWORD)(BufferData + Offset) = i; //位置
		// 记录数据大小的存放位置
		v11 = Offset + sizeof(DWORD);  //4
		v22 = v11 + sizeof(DWORD);     //8
		Count = 0; 
		//数据计数器归零

		//更新前一帧数据
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

			// 更新Dest中的数据
			*v1 = *v2;
			*(LPDWORD)(BufferData + v22 + Count) = *v2;
			Count += 4;
		}

		//Hex xoWorld   v1
		//Hex xoWorld   v2
		// 写入数据长度
		*(LPDWORD)(BufferData + v11) = Count;
		Offset = v22 + Count;
	}

	// nOffsetOffset 就是写入的总大小
	return Offset;
}

