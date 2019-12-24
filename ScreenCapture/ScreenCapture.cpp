// ScreenCapture.cpp : 定义应用程序的入口点。
/*
高DPI适应直接在项目属性页，清单工具，输入输出，DPI识别功能里设置

目前可能由于vs版本的问题无法使用以下方法
#include "ShellScalingAPI.h"	//高DPI头文件
#pragma comment(lib, "Shcore.lib")		//引入高DPI链接库
UINT dpiX = 0, dpiY = 0;
HMONITOR monitor = MonitorFromWindow(DeskWnd, MONITOR_DEFAULTTONEAREST);
GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
*/

#include "stdafx.h"
#include "ScreenCapture.h"
#include "Winuser.h"


#define MAX_LOADSTRING 100

// 全局变量: 
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

HDC g_srcMemDc;									// 原桌面截图
HDC g_grayMemDc;								//灰度桌面
int g_screenW;									// 屏幕宽
int g_screenH;									// 屏幕高
RECT g_rect;									// 鼠标选中的矩形区域
BOOL isSelected = false;						//是否已经选中
BOOL isDrawing = false;							//鼠标是否按下

// 此代码模块中包含的函数的前向声明: 
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

    // TODO: 在此放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SCREENCAPTURE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SCREENCAPTURE));

    MSG msg;

    // 主消息循环: 
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
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;	//可处理鼠标双击DBCLICKS
    wcex.lpfnWndProc    = WndProc;						//窗口过程函数
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
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   //参数：类名称，窗口标题，窗口弹出样式，窗口大小及位置(4)
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
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//回调函数，有消息触发调用
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;

	//画刷透明
	LOGBRUSH brush;
	brush.lbStyle = BS_NULL;
	HBRUSH hBrush = CreateBrushIndirect(&brush);
	//画笔
	LOGPEN pen;
	//画笔大小
	POINT ptPen;
	ptPen.x = 2;
	ptPen.y = 2;
	//画笔颜色，线条样式
	pen.lopnColor = 0x0000FFFF;
	pen.lopnStyle = PS_SOLID;
	pen.lopnWidth = ptPen;
	//创建画笔
	HPEN hPen = CreatePenIndirect(&pen);

    switch (message)
    {
	case WM_LBUTTONDOWN:	//鼠标左键点击
		{
			if (!isSelected)
			{
				POINT pt;
				GetCursorPos(&pt);		//获取光标位置
				g_rect.left = pt.x;
				g_rect.top = pt.y;
				g_rect.right = pt.x;
				g_rect.bottom = pt.y;

				isDrawing = true;
			}
			
		}
		break;
	case WM_LBUTTONUP:		//鼠标左键松开
		{
			if (isDrawing && !isSelected)
			{
				POINT pt;
				GetCursorPos(&pt);		//获取光标位置
				g_rect.right = pt.x;
				g_rect.bottom = pt.y;
				InvalidateRgn(hWnd, 0, true);	//刷新屏幕

				isDrawing = false;
				isSelected = true;
			}
			
		}
		break;
	case WM_MOUSEMOVE:		//鼠标移动
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
	case WM_LBUTTONDBLCLK:	//鼠标双击, 复制选中区域
		{
			if (isSelected)
			{
				CopyBitmapToCipBoard(ptPen);
				//窗口最小化到任务栏
				ShowWindow(hWnd, SW_MINIMIZE);
			}
			isSelected = false;
		}
		break;

    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择: 
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
            hdc = BeginPaint(hWnd, &ps);				//hdc当前窗口
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
			HDC memDc = CreateCompatibleDC(hdc);
			HBITMAP bmp = CreateCompatibleBitmap(hdc, g_screenW, g_screenH);
			SelectObject(memDc, bmp);

			BitBlt(memDc, 0, 0, g_screenW, g_screenH, g_grayMemDc, 0, 0, SRCCOPY);
			//传入透明画刷和轮廓画笔
			SelectObject(memDc, hBrush);
			SelectObject(memDc, hPen);

			//BitBlt(hdc, 0, 0, g_screenW, g_screenH, g_srcMemDc, 0, 0, SRCCOPY);

			//将截图区域的矩形和彩色图像传入memDc
			if (isDrawing || isSelected)
			{

				BitBlt(memDc, g_rect.left, g_rect.top, g_rect.right - g_rect.left, g_rect.bottom - g_rect.top, g_srcMemDc, g_rect.left, g_rect.top, SRCCOPY);
				Rectangle(memDc, g_rect.left, g_rect.top, g_rect.right, g_rect.bottom);
			}
			
			//全部传入程序主窗口DC
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

// “关于”框的消息处理程序。
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


//获取整个桌面的图像
void GetScreenCapture()
{
	//获取桌面句柄
	HWND DeskWnd=::GetDesktopWindow();
	//获取桌面DC
	HDC DeskDc = GetWindowDC(NULL);

	//获得整个桌面大小（分辨率）
	g_screenW = GetDeviceCaps(DeskDc, HORZRES);		//桌面水平大小
	g_screenH = GetDeviceCaps(DeskDc, VERTRES);		//桌面垂直大小

	//获取桌面截图
	g_srcMemDc = CreateCompatibleDC(DeskDc);					//创建一个兼容的DC(内存DC),与桌面DC相关
	//模拟一张画布
	HBITMAP hbitMap = CreateCompatibleBitmap(DeskDc, g_screenW, g_screenH);
	SelectObject(g_srcMemDc, hbitMap);							//将画布传入DC

	//将桌面DeskDc画到g_srcMemDc上
	BitBlt(g_srcMemDc, 0, 0, g_screenW, g_screenH, DeskDc, 0, 0, SRCCOPY);		//x1,y1,原图选取开始画的位置

	//获取屏幕的灰度图片																			
	g_grayMemDc = CreateCompatibleDC(DeskDc);
	HBITMAP grayMap = CreateCompatibleBitmap(DeskDc, g_screenW, g_screenH);  //模拟一张画布，其中是没有数据的
	SelectObject(g_grayMemDc, grayMap);   //将画图选入内存DC，其中还是没有数据的
	BitBlt(g_grayMemDc, 0, 0, g_screenW, g_screenH, DeskDc, 0, 0, SRCCOPY);  //将屏幕的dc中的画图，拷贝至内存DC中

	CovertToGrayBitmap(grayMap, g_grayMemDc);  //将彩色图片转换灰度图片

	DeleteObject(hbitMap);
	DeleteObject(grayMap);
	DeleteDC(DeskDc);

	return;
}

//复制选中区域到剪贴板
void CopyBitmapToCipBoard(POINT ptPen)
{
	//矩形区域
	int width = g_rect.right - g_rect.left - ptPen.x*3/2;
	int height = g_rect.bottom - g_rect.top - ptPen.y*2;

	HDC hSrcDc = ::CreateDC(L"DISPLAY", 0, 0, 0);
	HDC hMemDc = CreateCompatibleDC(hSrcDc);
	HBITMAP hBmp = CreateCompatibleBitmap(hSrcDc, width, height);	//模拟画布
	HBITMAP hOldMap = (HBITMAP)SelectObject(hMemDc, hBmp);	//画布选入DC

	//桌面选中的区域传入memDC
	BitBlt(hMemDc, 0, 0, width, height, hSrcDc, g_rect.left + ptPen.x/2, g_rect.top + ptPen.y/2, SRCCOPY);
	//得到区域位图的句柄
	HBITMAP hNewMap = (HBITMAP)SelectObject(hMemDc, hOldMap);

	//释放Dc
	DeleteDC(hMemDc);
	DeleteDC(hSrcDc);

	//传入剪贴板
	if (OpenClipboard(0))
	{
		EmptyClipboard();
		SetClipboardData(CF_BITMAP, hNewMap);
		CloseClipboard();
	}

	//释放HBITMAP
	DeleteObject(hBmp);
	DeleteObject(hOldMap);
	DeleteObject(hNewMap);
}

//截图背景转换为灰阶
void CovertToGrayBitmap(HBITMAP hSourceBmp, HDC sourceDc)
{
	HBITMAP retBmp = hSourceBmp;
	BITMAPINFO bmpInfo;
	ZeroMemory(&bmpInfo, sizeof(BITMAPINFO));
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

	//GetDIBits(HDC hdc, HBITMAP hbm, UINT start, UINT cLines, LPVOID lpvBits, LPBITMAPINFO lpbmi, UINT usage);
	//检索指定兼容位图的位，并使用指定的格式将它们复制到一个 DIB 缓冲区中
	//start 要检索的第一条扫描线; cLines要检索的扫描行数; 
	//lpvBits指向接收位图数据的缓冲区的指针。 如果此参数为 NULL，则函数将位图的尺寸和格式传递给lpbmi参数指向的BITMAPINFO 结构。
	//lpbmi指向 BITMAPINFO 结构的指针，该结构指定 DIB 数据的所需格式。

	//将原图复制到bmpInfo缓冲区
	GetDIBits(sourceDc, retBmp, 0, 0, NULL, &bmpInfo, DIB_RGB_COLORS);

	BYTE* bits = new BYTE[bmpInfo.bmiHeader.biSizeImage];
	//获取位图像素数据
	GetBitmapBits(retBmp, bmpInfo.bmiHeader.biSizeImage, bits);

	int bytePerPixel = 4;//默认32位
	if (bmpInfo.bmiHeader.biBitCount == 24)
	{
		bytePerPixel = 3;
	}
	//转换为灰度
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

