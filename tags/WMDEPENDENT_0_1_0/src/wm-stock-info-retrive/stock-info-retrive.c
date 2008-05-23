#include <windows.h>
#include <Connmgr.h>
#include <Mmsystem.h>
#include <commctrl.h>
#include <Winsock2.h>

#include "http-retriver.h"
#include "http-body-parse.h"

HANDLE g_hConnection;

static void
ws_cleanup (void)
{
	WSACleanup ();
	if ( g_hConnection )
	{
		ConnMgrReleaseConnection(g_hConnection, FALSE);
		g_hConnection = NULL;
	}
}

static void
set_sleep_mode (void)
{
	typedef DWORD (WINAPI *func_t) (DWORD);
	func_t set_exec_state;

	set_exec_state =
		(func_t) GetProcAddress (GetModuleHandle (_T("KERNEL32.DLL")),
		_T("SetThreadExecutionState"));

	if (set_exec_state)
		set_exec_state (ES_SYSTEM_REQUIRED | ES_CONTINUOUS);
}

int initialconnection()
{
	int nret = 0;
	DWORD dwStatus = 0;
	HRESULT hr;
	DWORD nIndex = 0;
	DWORD dwStartTime = 0;
	GUID guid = {0};
	CONNMGR_DESTINATION_INFO DestInfo = {0};
	CONNMGR_CONNECTIONINFO connInfo;

	ZeroMemory(&connInfo, sizeof(connInfo));

	for ( nIndex = 0; ; nIndex++ )
	{
		nret = 0;
		memset (&DestInfo, 0, sizeof(CONNMGR_DESTINATION_INFO) );
		if (ConnMgrEnumDestinations(nIndex, &DestInfo ) == E_FAIL )
			break;

		if (memcmp(DestInfo.szDescription, L"Internet", wcslen(L"Internet") * sizeof(TCHAR)) == 0 || memcmp(DestInfo.szDescription, L"internet",wcslen(L"internet")*sizeof(TCHAR)) == 0)
		{
			nret = 1;
			break;
		}
	}

	if (nret == 0)
	{
		hr = ConnMgrMapURL(_T("http://www.abacan.com/"), &guid, (DWORD*)&nIndex);
		if (FAILED(hr)) return 0;
		hr = ConnMgrEnumDestinations(nIndex, &DestInfo);
		if (FAILED(hr)) return 0;
	}

	connInfo.cbSize = sizeof(CONNMGR_CONNECTIONINFO);
	connInfo.dwParams = CONNMGR_PARAM_GUIDDESTNET;
	connInfo.dwPriority = CONNMGR_PRIORITY_USERINTERACTIVE;
	connInfo.bExclusive = FALSE;
	connInfo.bDisabled = FALSE;
	connInfo.dwFlags = CONNMGR_FLAG_PROXY_HTTP | CONNMGR_FLAG_PROXY_WAP | CONNMGR_FLAG_PROXY_SOCKS4 | CONNMGR_FLAG_PROXY_SOCKS5;
	connInfo.guidDestNet = DestInfo.guid;

	hr = ConnMgrEstablishConnectionSync(&connInfo, &g_hConnection, 10*1000, &dwStatus);
	if (FAILED(hr))
	{
		g_hConnection = NULL;
		return 0;
	}

	if (g_hConnection)
		dwStartTime = GetTickCount();
	while (GetTickCount() - dwStartTime < 10000)
	{
		if (g_hConnection)
		{
			if (g_hConnection)
			dwStatus = 0;
			hr = ConnMgrConnectionStatus (g_hConnection, &dwStatus);
			if (SUCCEEDED(hr))
			{
				if (dwStatus == CONNMGR_STATUS_CONNECTED)
				{
					nret = 1;
					break;
				}
			}
		}
		Sleep( 100 );
	}

	return nret;
}

/* Perform Windows specific initialization.  */
void
ws_startup (void)
{
	WSADATA data;
	WORD requested = MAKEWORD (1, 1);

	if (initialconnection() == 0)
	{
		MessageBox(NULL, L"程序初始化失败\n请检查GPRS 设置。", L"错误", MB_OK);
		exit(1);
	}

	if (WSAStartup (requested, &data) != 0)
	{
		MessageBox(NULL, L"程序初始化失败\n请检查GPRS 设置。", L"错误", MB_OK);
		exit(1);
	}

	if (data.wVersion < requested)
	{
		WSACleanup();
		MessageBox(NULL, L"程序初始化失败\n请检查GPRS 设置。", L"错误", MB_OK);
		exit(1);
	}

	atexit(ws_cleanup);
	set_sleep_mode();
	// SetConsoleCtrlHandler (ws_handler, TRUE);
}

void triggered_message(wchar_t* message, wchar_t* stock_code, struct stock_price* price_info)
{
	size_t covcount = 0;
	wchar_t* p = message;
	wcscpy(message, stock_code);
	for (; *p; p++);
	*(p++) = L'\n';
	covcount = mbstowcs(p, price_info->time_stamp, strlen(price_info->time_stamp));
	p += covcount;
	*(p++) = L'\n';
	swprintf(p,L"%.2f", price_info->current_price);
}

int _tmain(int argc, _TCHAR* argv[])
{
	wchar_t *stock_code_str;
	wchar_t *height_trigger_str;
	wchar_t *low_trigger_str;
	wchar_t *monitor_timeout_str;

	struct stock_price price_info;
	int loop_alarm = 0;
	double low_trigger = 0;
	double height_trigger = 0;
	double lowest_price = 0;
	double heighest_price  = 0;
	double monitor_timeout = 0;
	DWORD dwStartTime = 0;

	char *url = "http://hq.sinajs.cn/list=sh";
	char *full_url = NULL;
	char *p;
	wchar_t *wp;
	wchar_t message[256];
	wchar_t exe_path[256];
	int str_len;

	wcscpy(exe_path, argv[0]);
	wp = wcsrchr(exe_path, L'\\');
	str_len = &exe_path[255] - wp;
	memset((void *)++wp, 0, str_len * sizeof(wchar_t));
	wcscpy(wp, L"StockMew.exe");

	if (argc < 5)
		if (!CreateProcess(exe_path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, NULL, NULL))
			return -1;
		else
			return 0;

	ws_startup ();

	stock_code_str = argv[1];
	height_trigger_str = argv[2];
	low_trigger_str = argv[3];
	monitor_timeout_str = argv[4];

	full_url = (char*) malloc(strlen(url) + wcslen(stock_code_str) + 1);
	strcpy(full_url, url);
	for (p = full_url; *p ; p++);
	wcstombs(p, stock_code_str, wcslen(stock_code_str) + 1);
	low_trigger = wcstod(low_trigger_str, &wp);
	height_trigger = wcstod(height_trigger_str, &wp);
	monitor_timeout = wcstod(monitor_timeout_str, &wp);
	monitor_timeout = monitor_timeout < 120 ? monitor_timeout:120;

	if (low_trigger >= height_trigger)
	{
		return -1;
	}

	memset(&price_info, 0, sizeof(struct stock_price));
	dwStartTime = GetTickCount();
	while (1)
	{
		if (retrieve_stock_price(&price_info, full_url, parse_body_string_sina) == -1)
		{
			wcscpy(message, L"e 网络连接错误\n股价提醒软件自动退出。");
			CreateProcess(exe_path, message, NULL, NULL, FALSE, 0, NULL, NULL, NULL, NULL);
			return 0;
		}
		else
		{
			if (price_info.current_price >= height_trigger)
			{
				wcscpy(message, L"h ");
				triggered_message(&message[2], stock_code_str, &price_info);
				CreateProcess(exe_path, message, NULL, NULL, FALSE, 0, NULL, NULL, NULL, NULL);
				return 0;
			}

			if (price_info.current_price <= low_trigger)
			{
				wcscpy(message, L"l ");
				triggered_message(&message[2], stock_code_str, &price_info);
				CreateProcess(exe_path, message, NULL, NULL, FALSE, 0, NULL, NULL, NULL, NULL);
				return 0;
			}

			if (GetTickCount() - dwStartTime > 1000 * 60 * monitor_timeout)
			{
				wcscpy(message, L"t ");
				triggered_message(&message[2], stock_code_str, &price_info);
				CreateProcess(exe_path, message, NULL, NULL, FALSE, 0, NULL, NULL, NULL, NULL);
				return 0;
			}
		}
	}
}
