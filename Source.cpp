#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "setupapi")

#include <windows.h>
#include <setupapi.h>
#include <initguid.h>

#define NAME_SIZE 128

TCHAR szClassName[] = TEXT("Window");

DEFINE_GUID(GUID_CLASS_MONITOR, 0x4d36e96e, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18);

VOID PlayWithDeviceInfo(IN HDEVINFO devInfo, IN PSP_DEVINFO_DATA devInfoData, HWND hEdit)
{
	DWORD dwUIDNameSize = 0;
	SetupDiGetDeviceRegistryProperty(devInfo, devInfoData, SPDRP_DEVICEDESC, 0, 0, 0, &dwUIDNameSize);
	LPTSTR lpszUIDName = (LPTSTR)GlobalAlloc(0, dwUIDNameSize);
	if (lpszUIDName != NULL)
	{
		if (SetupDiGetDeviceRegistryProperty(devInfo, devInfoData, SPDRP_DEVICEDESC, 0, (BYTE*)lpszUIDName, dwUIDNameSize, 0))
		{
			SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)lpszUIDName);
			SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)TEXT("\r\n"));
		}
	}
	GlobalFree(lpszUIDName);
	HKEY hDevRegKey = SetupDiOpenDevRegKey(
		devInfo,
		devInfoData,
		DICS_FLAG_GLOBAL,
		0,
		DIREG_DEV,
		KEY_READ);
	if (hDevRegKey)
	{
		TCHAR valueName[NAME_SIZE];
		DWORD AcutalValueNameLength = _countof(valueName);
		for (LONG i = 0, retValue = ERROR_SUCCESS; retValue != ERROR_NO_MORE_ITEMS; i++)
		{
			unsigned char EDIDdata[1024];
			DWORD edidsize = _countof(EDIDdata);
			retValue = RegEnumValue(
				hDevRegKey,
				i,
				&valueName[0],
				&AcutalValueNameLength,
				NULL,
				NULL,
				EDIDdata,
				&edidsize);
			if (retValue == ERROR_SUCCESS)
			{
				if (!lstrcmp(valueName, TEXT("EDID")))
				{
					for (DWORD j = 0; j < edidsize; j++)
					{
						if (j % 16 == 0 && j != 0)
						{
							SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)TEXT("\r\n"));
						}
						TCHAR szText[4];
						wsprintf(szText, TEXT("%02x "), EDIDdata[j]);
						SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)szText);
					}
					SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)TEXT("\r\n"));
					break;
				}
			}
		}
		RegCloseKey(hDevRegKey);
	}
	SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)TEXT("\r\n"));
}

int EnumDevices(HWND hEdit)
{
	HDEVINFO devInfo = SetupDiGetClassDevsEx(&GUID_CLASS_MONITOR, NULL, NULL, DIGCF_PRESENT, NULL, NULL, NULL);
	if (devInfo)
	{
		for (int i = 0; ERROR_NO_MORE_ITEMS != GetLastError(); ++i)
		{
			SP_DEVINFO_DATA devInfoData = { sizeof(devInfoData) };
			if (SetupDiEnumDeviceInfo(devInfo, i, &devInfoData))
			{
				PlayWithDeviceInfo(devInfo, &devInfoData, hEdit);
			}
		}
	}
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hButton;
	static HWND hEdit;
	static HFONT hFont;
	switch (msg)
	{
	case WM_CREATE:
		hFont = CreateFontW(-MulDiv(10, 96, 72), 0, 0, 0, FW_NORMAL, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0, 0, L"ＭＳ ゴシック");
		hButton = CreateWindow(TEXT("BUTTON"), TEXT("EDID を取得"), WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hWnd, (HMENU)IDOK, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, 0);
		hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), 0, WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, 0);
		break;
	case WM_SIZE:
		MoveWindow(hButton, 10, 10, 256, 32, TRUE);
		MoveWindow(hEdit, 10, 50, LOWORD(lParam) - 20, HIWORD(lParam) - 60, TRUE);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			SetWindowText(hEdit, 0);
			EnumDevices(hEdit);
			SendMessage(hEdit, EM_SETSEL, 0, -1);
			SetFocus(hEdit);
		}
		break;
	case WM_DESTROY:
		DeleteObject(hFont);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("ディスプレイの EDID を取得"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
