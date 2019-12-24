// ScreenCapture.cpp : ����Ӧ�ó������ڵ㡣
/*
��DPI��Ӧֱ������Ŀ����ҳ���嵥���ߣ����������DPIʶ����������

Ŀǰ��������vs�汾�������޷�ʹ�����·���
#include "ShellScalingAPI.h"	//��DPIͷ�ļ�
#pragma comment(lib, "Shcore.lib")		//�����DPI���ӿ�
UINT dpiX = 0, dpiY = 0;
HMONITOR monitor = MonitorFromWindow(DeskWnd, MONITOR_DEFAULTTONEAREST);
GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
*/

#include "stdafx.h"
#include "ScreenCapture.h"
#include "Winuser.h"


#define MAX_LOADSTRING 100

// ȫ�ֱ���: 
HINSTANCE hInst;                                // ��ǰʵ��
WCHAR szTitle[MAX_LOADSTRING];                  // �������ı�
WCHAR szWindowClass[MAX_LOADSTRING];            // ����������

HDC g_srcMemDc;									// ԭ�����ͼ
HDC g_grayMemDc;								//�Ҷ�����
int g_screenW;									// ��Ļ��
int g_screenH;									// ��Ļ��
RECT g_rect;									// ���ѡ�еľ�������
BOOL isSelected = false;						//�Ƿ��Ѿ�ѡ��
BOOL isDrawing = false;							//����Ƿ���

// �˴���ģ���а����ĺ�����ǰ������: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void GetScreenCapture();
void CopyBitmapToCipBoard(POINT ptPen);
void CovertToGrayBitmap(HBITMAP hSourceBmp, HDC sourceDc);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: �ڴ˷��ô��롣

    // ��ʼ��ȫ���ַ���
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SCREENCAPTURE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // ִ��Ӧ�ó����ʼ��: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SCREENCAPTURE));

    MSG msg;

    // ����Ϣѭ��: 
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  ����: MyRegisterClass()
//
//  Ŀ��: ע�ᴰ���ࡣ
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;	//�ɴ������˫��DBCLICKS
    wcex.lpfnWndProc    = WndProc;						//���ڹ��̺���
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SCREENCAPTURE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = 0; //MAKEINTRESOURCEW(IDC_SCREENCAPTURE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   ����: InitInstance(HINSTANCE, int)
//
//   Ŀ��: ����ʵ�����������������
//
//   ע��: 
//
//        �ڴ˺����У�������ȫ�ֱ����б���ʵ�������
//        ��������ʾ�����򴰿ڡ�
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // ��ʵ������洢��ȫ�ֱ�����

   //�����������ƣ����ڱ��⣬���ڵ�����ʽ�����ڴ�С��λ��(4)
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_POPUP,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, SW_SHOWMAXIMIZED);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  ����: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  Ŀ��:    ���������ڵ���Ϣ��
//
//  WM_COMMAND  - ����Ӧ�ó���˵�
//  WM_PAINT    - ����������
//  WM_DESTROY  - �����˳���Ϣ������
//
//�ص�����������Ϣ��������
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;

	//��ˢ͸��
	LOGBRUSH brush;
	brush.lbStyle = BS_NULL;
	HBRUSH hBrush = CreateBrushIndirect(&brush);
	//����
	LOGPEN pen;
	//���ʴ�С
	POINT ptPen;
	ptPen.x = 2;
	ptPen.y = 2;
	//������ɫ��������ʽ
	pen.lopnColor = 0x0000FFFF;
	pen.lopnStyle = PS_SOLID;
	pen.lopnWidth = ptPen;
	//��������
	HPEN hPen = CreatePenIndirect(&pen);

    switch (message)
    {
	case WM_LBUTTONDOWN:	//���������
		{
			if (!isSelected)
			{
				POINT pt;
				GetCursorPos(&pt);		//��ȡ���λ��
				g_rect.left = pt.x;
				g_rect.top = pt.y;
				g_rect.right = pt.x;
				g_rect.bottom = pt.y;

				isDrawing = true;
			}
			
		}
		break;
	case WM_LBUTTONUP:		//�������ɿ�
		{
			if (isDrawing && !isSelected)
			{
				POINT pt;
				GetCursorPos(&pt);		//��ȡ���λ��
				g_rect.right = pt.x;
				g_rect.bottom = pt.y;
				InvalidateRgn(hWnd, 0, true);	//ˢ����Ļ

				isDrawing = false;
				isSelected = true;
			}
			
		}
		break;
	case WM_MOUSEMOVE:		//����ƶ�
		{
			if (isDrawing && !isSelected)
			{
				POINT pt;
				GetCursorPos(&pt);
				g_rect.right = pt.x;
				g_rect.bottom = pt.y;
				InvalidateRgn(hWnd, 0, false);
			}
		}
		break;
	case WM_LBUTTONDBLCLK:	//���˫��, ����ѡ������
		{
			if (isSelected)
			{
				CopyBitmapToCipBoard(ptPen);
				//������С����������
				ShowWindow(hWnd, SW_MINIMIZE);
			}
			isSelected = false;
		}
		break;

    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // �����˵�ѡ��: 
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
	case WM_CREATE:
		GetScreenCapture();
		break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            hdc = BeginPaint(hWnd, &ps);				//hdc��ǰ����
            // TODO: �ڴ˴����ʹ�� hdc ���κλ�ͼ����...
			HDC memDc = CreateCompatibleDC(hdc);
			HBITMAP bmp = CreateCompatibleBitmap(hdc, g_screenW, g_screenH);
			SelectObject(memDc, bmp);

			BitBlt(memDc, 0, 0, g_screenW, g_screenH, g_grayMemDc, 0, 0, SRCCOPY);
			//����͸����ˢ����������
			SelectObject(memDc, hBrush);
			SelectObject(memDc, hPen);

			//BitBlt(hdc, 0, 0, g_screenW, g_screenH, g_srcMemDc, 0, 0, SRCCOPY);

			//����ͼ����ľ��κͲ�ɫͼ����memDc
			if (isDrawing || isSelected)
			{

				BitBlt(memDc, g_rect.left, g_rect.top, g_rect.right - g_rect.left, g_rect.bottom - g_rect.top, g_srcMemDc, g_rect.left, g_rect.top, SRCCOPY);
				Rectangle(memDc, g_rect.left, g_rect.top, g_rect.right, g_rect.bottom);
			}
			
			//ȫ���������������DC
			BitBlt(hdc, 0, 0, g_screenW, g_screenH, memDc, 0, 0, SRCCOPY);

			DeleteObject(bmp);
			DeleteDC(memDc);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// �����ڡ������Ϣ�������
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


//��ȡ���������ͼ��
void GetScreenCapture()
{
	//��ȡ������
	HWND DeskWnd=::GetDesktopWindow();
	//��ȡ����DC
	HDC DeskDc = GetWindowDC(NULL);

	//������������С���ֱ��ʣ�
	g_screenW = GetDeviceCaps(DeskDc, HORZRES);		//����ˮƽ��С
	g_screenH = GetDeviceCaps(DeskDc, VERTRES);		//���洹ֱ��С

	//��ȡ�����ͼ
	g_srcMemDc = CreateCompatibleDC(DeskDc);					//����һ�����ݵ�DC(�ڴ�DC),������DC���
	//ģ��һ�Ż���
	HBITMAP hbitMap = CreateCompatibleBitmap(DeskDc, g_screenW, g_screenH);
	SelectObject(g_srcMemDc, hbitMap);							//����������DC

	//������DeskDc����g_srcMemDc��
	BitBlt(g_srcMemDc, 0, 0, g_screenW, g_screenH, DeskDc, 0, 0, SRCCOPY);		//x1,y1,ԭͼѡȡ��ʼ����λ��

	//��ȡ��Ļ�ĻҶ�ͼƬ																			
	g_grayMemDc = CreateCompatibleDC(DeskDc);
	HBITMAP grayMap = CreateCompatibleBitmap(DeskDc, g_screenW, g_screenH);  //ģ��һ�Ż�����������û�����ݵ�
	SelectObject(g_grayMemDc, grayMap);   //����ͼѡ���ڴ�DC�����л���û�����ݵ�
	BitBlt(g_grayMemDc, 0, 0, g_screenW, g_screenH, DeskDc, 0, 0, SRCCOPY);  //����Ļ��dc�еĻ�ͼ���������ڴ�DC��

	CovertToGrayBitmap(grayMap, g_grayMemDc);  //����ɫͼƬת���Ҷ�ͼƬ

	DeleteObject(hbitMap);
	DeleteObject(grayMap);
	DeleteDC(DeskDc);

	return;
}

//����ѡ�����򵽼�����
void CopyBitmapToCipBoard(POINT ptPen)
{
	//��������
	int width = g_rect.right - g_rect.left - ptPen.x*3/2;
	int height = g_rect.bottom - g_rect.top - ptPen.y*2;

	HDC hSrcDc = ::CreateDC(L"DISPLAY", 0, 0, 0);
	HDC hMemDc = CreateCompatibleDC(hSrcDc);
	HBITMAP hBmp = CreateCompatibleBitmap(hSrcDc, width, height);	//ģ�⻭��
	HBITMAP hOldMap = (HBITMAP)SelectObject(hMemDc, hBmp);	//����ѡ��DC

	//����ѡ�е�������memDC
	BitBlt(hMemDc, 0, 0, width, height, hSrcDc, g_rect.left + ptPen.x/2, g_rect.top + ptPen.y/2, SRCCOPY);
	//�õ�����λͼ�ľ��
	HBITMAP hNewMap = (HBITMAP)SelectObject(hMemDc, hOldMap);

	//�ͷ�Dc
	DeleteDC(hMemDc);
	DeleteDC(hSrcDc);

	//���������
	if (OpenClipboard(0))
	{
		EmptyClipboard();
		SetClipboardData(CF_BITMAP, hNewMap);
		CloseClipboard();
	}

	//�ͷ�HBITMAP
	DeleteObject(hBmp);
	DeleteObject(hOldMap);
	DeleteObject(hNewMap);
}

//��ͼ����ת��Ϊ�ҽ�
void CovertToGrayBitmap(HBITMAP hSourceBmp, HDC sourceDc)
{
	HBITMAP retBmp = hSourceBmp;
	BITMAPINFO bmpInfo;
	ZeroMemory(&bmpInfo, sizeof(BITMAPINFO));
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

	//GetDIBits(HDC hdc, HBITMAP hbm, UINT start, UINT cLines, LPVOID lpvBits, LPBITMAPINFO lpbmi, UINT usage);
	//����ָ������λͼ��λ����ʹ��ָ���ĸ�ʽ�����Ǹ��Ƶ�һ�� DIB ��������
	//start Ҫ�����ĵ�һ��ɨ����; cLinesҪ������ɨ������; 
	//lpvBitsָ�����λͼ���ݵĻ�������ָ�롣 ����˲���Ϊ NULL��������λͼ�ĳߴ�͸�ʽ���ݸ�lpbmi����ָ���BITMAPINFO �ṹ��
	//lpbmiָ�� BITMAPINFO �ṹ��ָ�룬�ýṹָ�� DIB ���ݵ������ʽ��

	//��ԭͼ���Ƶ�bmpInfo������
	GetDIBits(sourceDc, retBmp, 0, 0, NULL, &bmpInfo, DIB_RGB_COLORS);

	BYTE* bits = new BYTE[bmpInfo.bmiHeader.biSizeImage];
	//��ȡλͼ��������
	GetBitmapBits(retBmp, bmpInfo.bmiHeader.biSizeImage, bits);

	int bytePerPixel = 4;//Ĭ��32λ
	if (bmpInfo.bmiHeader.biBitCount == 24)
	{
		bytePerPixel = 3;
	}
	//ת��Ϊ�Ҷ�
	for (DWORD i = 0; i<bmpInfo.bmiHeader.biSizeImage; i += bytePerPixel)
	{
		BYTE r = *(bits + i);
		BYTE g = *(bits + i + 1);
		BYTE b = *(bits + i + 2);
		*(bits + i) = *(bits + i + 1) = *(bits + i + 2) = (r + b + g) / 3;
	}
	SetBitmapBits(hSourceBmp, bmpInfo.bmiHeader.biSizeImage, bits);
	delete[] bits;
}

