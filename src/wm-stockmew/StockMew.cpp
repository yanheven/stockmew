// StockMew.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "StockMew.h"
#include <windows.h>
#include <commctrl.h>
#include <tpcshell.h>
#include <Tlhelp32.h>
#include <Winuser.h>
#include <Mmsystem.h>
#include "StockMew-Utils.h"

#pragma comment(lib,"Toolhelp.lib")

#define MAX_LOADSTRING 100

#define ARRAY_LENGTH(x) (sizeof(x)/sizeof((x)[0]))

BOOL CALLBACK SetDialogProc(const HWND hDlg, const UINT uiMessage, 
						 const WPARAM wParam, const LPARAM lParam);
BOOL CALLBACK SignalDialogProc(const HWND hDlg, const UINT uiMessage, 
						 const WPARAM wParam, const LPARAM lParam);
HANDLE GetMonitorHANDLE();

// Global Variables:
HINSTANCE			g_hInst;			// current instance
HWND				g_hWndMenuBar;		// menu bar handle
LPTSTR				lpMessage;			// display message

static BOOL IsSmartphone() 
{
    TCHAR tszPlatform[64];

    if (TRUE == SystemParametersInfo(SPI_GETPLATFORMTYPE,
         sizeof(tszPlatform)/sizeof(*tszPlatform),tszPlatform,0))
    {
        if (0 == _tcsicmp(TEXT("Smartphone"), tszPlatform)) 
        {
            return TRUE;
        }
    }
    return FALSE;   
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPTSTR    lpCmdLine,
                   int       nCmdShow)
{
	// flag
	int iTryDial = 1;

	// store the hInstance
	g_hInst = hInstance;
	
	lpMessage = lpCmdLine;
	if (lpMessage && wcslen(lpMessage) > 0)
		iTryDial = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_STOCKMEW_SIGNAL), 0, (DLGPROC)SignalDialogProc);
	if (iTryDial == 1)
		DialogBox(g_hInst, MAKEINTRESOURCE(IDD_STOCKMEW_SET), 0, (DLGPROC)SetDialogProc);
	
	return 0;
}

BOOL CALLBACK SetDialogProc(const HWND hDlg, const UINT uiMessage, 
						 const WPARAM wParam, const LPARAM lParam)
{
	DWORD	dwFlags = 0;
	BOOL bProcessedMsg = true;

	switch(uiMessage)
	{
		case WM_INITDIALOG:
            // This is a standard message received before the dialog
            // box is displayed so initialise and set up the resources.

            // Specify that the dialog box should stretch full screen
			SHINITDLGINFO shidi;
			
			ZeroMemory(&shidi, sizeof(shidi));
            
			dwFlags = SHIDIF_SIZEDLGFULLSCREEN;

			if (!IsSmartphone())
			{
				dwFlags |= SHIDIF_DONEBUTTON;
			}
			shidi.dwMask = SHIDIM_FLAGS;
            shidi.dwFlags = dwFlags;
            shidi.hDlg = hDlg;
            
			// Set up the menu bar
			SHMENUBARINFO shmbi;
			ZeroMemory(&shmbi, sizeof(shmbi));
            shmbi.cbSize = sizeof(shmbi);
            shmbi.hwndParent = hDlg;
            shmbi.nToolBarId = IDR_STOCKMEW_SETMENU;
            shmbi.hInstRes = g_hInst;
			shmbi.dwFlags |= SHCMBF_HMENU;

			// If we could not initialize the dialog box, return an error
			if (!(SHInitDialog(&shidi) && SHCreateMenuBar(&shmbi))) 
            {
				EndDialog(hDlg, -1);
			}
            else
			// set the title bar 
            {
                TCHAR sz[160]; 
                LoadString(g_hInst, IDS_STOCKMEW_TITLE, sz, ARRAY_LENGTH(sz)); 
                SetWindowText(hDlg, sz); 
            } 

			(void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK,
				MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY,
				SHMBOF_NODEFAULT | SHMBOF_NOTIFY));

			SetDlgItemText(hDlg, IDC_EDIT_MONITOR_TIMEOUT, L"0.5");

			break;
		case WM_COMMAND:
            // An event has occured in the dialog box
            // The low-order word of wParam specifies the identifier 
            // of the menu item, control, or accelerator.
			switch (LOWORD(wParam)) 
			{
			case IDOK:
			case ID_START_MONITOR:
				{
					TCHAR   *szPath = NULL;
					TCHAR exepath[255];
					TCHAR *szCmdLine = NULL;
					TCHAR cmdLine[255];
					char tmp[255];
					int nRet = 0;
					int nRemain = 255;
					int nCmdLineLen = 0;

					szCmdLine = cmdLine;
					nRet = GetDlgItemText(hDlg, IDC_EDIT_STOCK_CODE, szCmdLine + nCmdLineLen, nRemain - 1);
					wcstombs(tmp, szCmdLine + nCmdLineLen, 255);
					if (nRet != 0 && verifystockcodeexpress(tmp))
					{
						nRemain -= nRet;
						nCmdLineLen += nRet;
						szCmdLine[nCmdLineLen++] = L' ';
					}
					else
					{
						MessageBox(NULL, L"股票代码输入错误。", L"错误", MB_OK);
						break;
					}
					nRet = GetDlgItemText(hDlg, IDC_EDIT_HIGHT_TRIGGER, szCmdLine + nCmdLineLen, nRemain - 1);
					wcstombs(tmp, szCmdLine + nCmdLineLen, 255);
					if (nRet != 0 && verifydoubleexpress(tmp))
					{
						nRemain -= nRet;
						nCmdLineLen += nRet;
						szCmdLine[nCmdLineLen++] = L' ';
					}
					else
					{
						MessageBox(NULL, L"高价提醒值输入错误。", L"错误", MB_OK);
						break;
					}
					nRet = GetDlgItemText(hDlg, IDC_EDIT_LOW_TRIGGER, szCmdLine + nCmdLineLen, nRemain - 1);
					wcstombs(tmp, szCmdLine + nCmdLineLen, 255);
					if (nRet != 0 && verifydoubleexpress(tmp))
					{
						nRemain -= nRet;
						nCmdLineLen += nRet;
						szCmdLine[nCmdLineLen++] = L' ';
					}
					else
					{
						MessageBox(NULL, L"低价提醒值输入错误。", L"错误", MB_OK);
						break;
					}
					nRet = GetDlgItemText(hDlg, IDC_EDIT_MONITOR_TIMEOUT, szCmdLine + nCmdLineLen, nRemain - 1);
					wcstombs(tmp, szCmdLine + nCmdLineLen, 255);
					if (nRet != 0 && verifydoubleexpress(tmp))
					{
						nRemain -= nRet;
						nCmdLineLen += nRet;
						szCmdLine[nCmdLineLen++] = L'\0';
					}
					else
					{
						MessageBox(NULL, L"监控时间输入错误。", L"错误", MB_OK);
						break;
					}

					if (IDCANCEL == MessageBox(NULL, L"软件运行将产生GPRS流量。\n程序运行过程中参数不能修改，请您确认已认真阅读随软件发布的README.TXT和HOWTO.TXT文件，并正确设置了参数。", L"警告", MB_ICONWARNING | MB_OKCANCEL))
						break;

					GetModuleFileNameW(NULL,exepath,sizeof(exepath)/sizeof(TCHAR));
					szPath = wcsrchr(exepath,L'\\');
					if ( szPath != NULL ) 
						*szPath = 0x00;
					szPath = wcscat(exepath,TEXT("\\retrive.exe"));

					if (!CreateProcess(szPath, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, NULL, NULL))
						MessageBox(NULL, L"股价提醒软件启动失败。", L"错误", MB_OK);
					else
						EndDialog(hDlg, 0);

					break;
				}
			case IDC_STATIC_HOMEPAGE:
				{
					SHELLEXECUTEINFO execInf;
					ZeroMemory (&execInf, sizeof (execInf));
					execInf.cbSize = sizeof (execInf);
					execInf.fMask = SEE_MASK_NOCLOSEPROCESS;
					execInf.lpFile = TEXT("http://hi.baidu.com/abacan");
					execInf.lpVerb = TEXT("open");
					ShellExecuteEx (&execInf);
					break;
				}
			case IDM_STOCKMEW_CANCEL:
				// do nothing
				EndDialog(hDlg, 0);
				break;
			}
			break;
		case WM_HOTKEY:
			if(VK_TBACK == HIWORD(lParam))
			{
				SHSendBackToFocusWindow(uiMessage, wParam, lParam);
			}
			break;
		default:
			// nothing was processed
			bProcessedMsg = false;
			break;
	}

	return bProcessedMsg;
}

BOOL CALLBACK SignalDialogProc(const HWND hDlg, const UINT uiMessage, 
						 const WPARAM wParam, const LPARAM lParam)
{
	DWORD	dwFlags = 0;
	BOOL bProcessedMsg = true;

	if ((uiMessage >= 0x0100 && uiMessage <= 0x0108) || uiMessage == WM_HOTKEY)
		PlaySound(NULL, g_hInst, 0x0040); 

	switch(uiMessage)
	{
		case WM_INITDIALOG:
            // This is a standard message received before the dialog
            // box is displayed so initialise and set up the resources.

            // Specify that the dialog box should stretch full screen
			SHINITDLGINFO shidi;
			
			ZeroMemory(&shidi, sizeof(shidi));
            
			dwFlags = SHIDIF_SIZEDLGFULLSCREEN;

			if (!IsSmartphone())
			{
				dwFlags |= SHIDIF_DONEBUTTON;
			}
			shidi.dwMask = SHIDIM_FLAGS;
            shidi.dwFlags = dwFlags;
            shidi.hDlg = hDlg;
            
			// Set up the menu bar
			SHMENUBARINFO shmbi;
			ZeroMemory(&shmbi, sizeof(shmbi));
            shmbi.cbSize = sizeof(shmbi);
            shmbi.hwndParent = hDlg;
            shmbi.nToolBarId = IDR_STOCKMEW_SIGNALMENU;
            shmbi.hInstRes = g_hInst;
			shmbi.dwFlags |= SHCMBF_HMENU;

			// If we could not initialize the dialog box, return an error
			if (!(SHInitDialog(&shidi) && SHCreateMenuBar(&shmbi))) 
            {
				EndDialog(hDlg, -1);
			}
            else
			// set the title bar 
            {
                TCHAR sz[160]; 
                LoadString(g_hInst, IDS_STOCKMEW_TITLE, sz, ARRAY_LENGTH(sz)); 
                SetWindowText(hDlg, sz); 
            }

			if (*lpMessage == L'e' || *lpMessage == L't')
				PlaySound(MAKEINTRESOURCE(IDR_WAVE2), g_hInst, SND_RESOURCE | SND_SYNC);
			else if (*lpMessage == L'h' || *lpMessage == L'l')
				PlaySound(MAKEINTRESOURCE(IDR_WAVE1), g_hInst, SND_RESOURCE | SND_SYNC);

			for (;*lpMessage != L' ' && *lpMessage != L'\0'; lpMessage++);
			SetDlgItemText(hDlg, IDC_STATIC_SIGNAL, lpMessage);
			SetDlgItemText(hDlg, IDC_STATIC_ILLUSTRATE, L"股价提醒软件已停止对股价监控\n您可选择退出程序或重新开始监控。");

			(void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK,
				MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY,
				SHMBOF_NODEFAULT | SHMBOF_NOTIFY));

			break;
		case WM_COMMAND:
            // An event has occured in the dialog box
            // The low-order word of wParam specifies the identifier 
            // of the menu item, control, or accelerator.
			switch (LOWORD(wParam)) 
			{
			case ID_RESTART_MONITOR:
				EndDialog(hDlg, 1);
				break;
			case IDM_SIGNAL_CANCEL:
			case IDOK:
				// do nothing
				EndDialog(hDlg, 0);
				break;
			}
			break;
		case WM_HOTKEY:
			if(VK_TBACK == HIWORD(lParam))
			{
				SHSendBackToFocusWindow(uiMessage, wParam, lParam);
			}
			break;
		default:
			// nothing was processed
			bProcessedMsg = false;
			break;
	}

	return bProcessedMsg;
}


HANDLE GetMonitorHANDLE()
{
	PROCESSENTRY32   pe; 
	pe.dwSize = sizeof(PROCESSENTRY32); 
	DWORD   dwProcessID = 0; 
	HANDLE hr = 0;

	HANDLE   hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
	Process32First(hSnapshot,   &pe); 

	do{ 
		if(0 == wcscmp(_T( "test.exe"), pe.szExeFile)) 
		{ 
			hr = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID); 
			break; 
		} 

		pe.dwSize = sizeof(PROCESSENTRY32); 
	}while   (Process32Next(hSnapshot, &pe)); 

	CloseToolhelp32Snapshot(hSnapshot);

	return hr;
}