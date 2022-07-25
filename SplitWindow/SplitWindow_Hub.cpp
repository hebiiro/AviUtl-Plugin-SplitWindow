#include "pch.h"
#include "SplitWindow.h"
#include "ConfigDialog.h"

//---------------------------------------------------------------------

// 他のウィンドウの土台となるシングルウィンドウを作成する。
HWND createHub()
{
	MY_TRACE(_T("createHub()\n"));

	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.hCursor = ::LoadCursor(0, IDC_ARROW);
	wc.lpfnWndProc = hubProc;
	wc.hInstance = g_instance;
	wc.lpszClassName = _T("AviUtl"); // クラス名を AviUtl に偽装する。「AoiSupport」用。
	::RegisterClass(&wc);

	HWND hwnd = ::CreateWindowEx(
		0,
		_T("SplitWindow"), // フックして AviUtl に置き換える。
		_T("SplitWindow"),
		WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME |
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		0, 0, g_instance, 0);

	return hwnd;
}

//---------------------------------------------------------------------

// ファイルからレイアウトをインポートする。
BOOL importLayout(HWND hwnd)
{
	// ファイル選択ダイアログを表示してファイル名を取得する。

	WCHAR fileName[MAX_PATH] = {};

	WCHAR folderName[MAX_PATH] = {};
	::GetModuleFileNameW(g_instance, folderName, MAX_PATH);
	::PathRemoveExtensionW(folderName);

	OPENFILENAMEW ofn = { sizeof(ofn) };
	ofn.hwndOwner = hwnd;
	ofn.Flags = OFN_FILEMUSTEXIST;
	ofn.lpstrTitle = L"レイアウトのインポート";
	ofn.lpstrInitialDir = folderName;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = L"レイアウトファイル (*.xml)\0*.xml\0" "すべてのファイル (*.*)\0*.*\0";
	ofn.lpstrDefExt = L"xml";

	if (!::GetOpenFileNameW(&ofn))
		return FALSE;

	// レイアウトファイルをインポートする。
	loadConfig(fileName, TRUE);

	// レイアウトを再計算する。
	calcAllLayout();

	// シングルウィンドウが非表示なら表示する。
	if (!::IsWindowVisible(hwnd))
		::ShowWindow(hwnd, SW_SHOW);

	// シングルウィンドウをフォアグラウンドにする。
	::SetForegroundWindow(hwnd);
	::SetActiveWindow(hwnd);

	return TRUE;
}

// ファイルにレイアウトをエクスポートする。
BOOL exportLayout(HWND hwnd)
{
	// ファイル選択ダイアログを表示してファイル名を取得する。

	WCHAR fileName[MAX_PATH] = {};

	WCHAR folderName[MAX_PATH] = {};
	::GetModuleFileNameW(g_instance, folderName, MAX_PATH);
	::PathRemoveExtensionW(folderName);

	OPENFILENAMEW ofn = { sizeof(ofn) };
	ofn.hwndOwner = hwnd;
	ofn.Flags = OFN_OVERWRITEPROMPT;
	ofn.lpstrTitle = L"レイアウトのエクスポート";
	ofn.lpstrInitialDir = folderName;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = L"レイアウトファイル (*.xml)\0*.xml\0" "すべてのファイル (*.*)\0*.*\0";
	ofn.lpstrDefExt = L"xml";

	if (!::GetSaveFileNameW(&ofn))
		return FALSE;

	// レイアウトファイルをエクスポートする。
	saveConfig(fileName, TRUE);

	return TRUE;
}

//---------------------------------------------------------------------

// シングルウィンドウのウィンドウ関数。
LRESULT CALLBACK hubProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_CHAR:
	case WM_DEADCHAR:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_SYSCHAR:
	case WM_SYSDEADCHAR:
		{
			MY_TRACE(_T("hubProc(WM_***KEY***, 0x%08X, 0x%08X, 0x%08X)\n"), message, wParam, lParam);

			return ::SendMessage(g_aviutlWindow->m_hwnd, message, wParam, lParam);
		}
	case WM_ACTIVATE: // 「patch.aul」用。
		{
			MY_TRACE(_T("hubProc(WM_ACTIVATE, 0x%08X, 0x%08X)\n"), wParam, lParam);

			if (LOWORD(wParam) == WA_INACTIVE)
				::SendMessage(g_aviutlWindow->m_hwnd, message, wParam, lParam);

			break;
		}
	case WM_MENUSELECT: // 「patch.aul」用。
		{
			MY_TRACE(_T("hubProc(WM_MENUSELECT, 0x%08X, 0x%08X)\n"), wParam, lParam);

			::SendMessage(g_aviutlWindow->m_hwnd, message, wParam, lParam);

			break;
		}
	case WM_CLOSE:
		{
			MY_TRACE(_T("hubProc(WM_CLOSE, 0x%08X, 0x%08X)\n"), wParam, lParam);

			return ::SendMessage(g_aviutlWindow->m_hwnd, message, wParam, lParam);
		}
	case WM_COMMAND:
		{
			MY_TRACE(_T("hubProc(WM_COMMAND, 0x%08X, 0x%08X)\n"), wParam, lParam);

			return ::SendMessage(g_aviutlWindow->m_hwnd, message, wParam, lParam);
		}
	case WM_SYSCOMMAND:
		{
			MY_TRACE(_T("hubProc(WM_SYSCOMMAND, 0x%08X, 0x%08X)\n"), wParam, lParam);

			switch (wParam)
			{
			case CommandID::SHOW_CONFIG_DIALOG:
				{
					// SplitWindow の設定ダイアログを開く。
					showConfigDialog(hwnd);

					break;
				}
			case CommandID::IMPORT_LAYOUT:
				{
					// レイアウトファイルをインポートする。
					importLayout(hwnd);

					break;
				}
			case CommandID::EXPORT_LAYOUT:
				{
					// レイアウトファイルをエクスポートする。
					exportLayout(hwnd);

					break;
				}
			case CommandID::CREATE_COLONY:
				{
					// コロニーを新規作成する。
					createColony();

					break;
				}
			case SC_RESTORE:
			case SC_MINIMIZE: // 「patch.aul」用。
				{
					::SendMessage(g_aviutlWindow->m_hwnd, message, wParam, lParam);

					break;
				}
			}

			break;
		}
	case WM_CREATE:
		{
			MY_TRACE(_T("hubProc(WM_CREATE)\n"));

			g_theme = ::OpenThemeData(hwnd, VSCLASS_WINDOW);
			MY_TRACE_HEX(g_theme);

			HMENU menu = ::GetSystemMenu(hwnd, FALSE);
			::InsertMenu(menu, 0, MF_BYPOSITION | MF_STRING, CommandID::CREATE_COLONY, _T("コロニーを新規作成"));
			::InsertMenu(menu, 1, MF_BYPOSITION | MF_STRING, CommandID::IMPORT_LAYOUT, _T("レイアウトのインポート"));
			::InsertMenu(menu, 2, MF_BYPOSITION | MF_STRING, CommandID::EXPORT_LAYOUT, _T("レイアウトのエクスポート"));
			::InsertMenu(menu, 3, MF_BYPOSITION | MF_STRING, CommandID::SHOW_CONFIG_DIALOG, _T("SplitWindowの設定"));
			::InsertMenu(menu, 4, MF_BYPOSITION | MF_SEPARATOR, 0, 0);

			break;
		}
	case WM_DESTROY:
		{
			MY_TRACE(_T("hubProc(WM_DESTROY)\n"));

			::CloseThemeData(g_theme), g_theme = 0;

			break;
		}
	case WindowMessage::WM_POST_INIT: // 最後の初期化処理。
		{
			// 設定をファイルから読み込む。
			loadConfig();

			// シングルウィンドウが非表示なら表示する。
			if (!::IsWindowVisible(hwnd))
				::ShowWindow(hwnd, SW_SHOW);

			// 最初のレイアウト計算。
			calcAllLayout();

			// シングルウィンドウをフォアグラウンドにする。
			::SetForegroundWindow(hwnd);
			::SetActiveWindow(hwnd);

			break;
		}
	}

	return colonyProc(hwnd, message, wParam, lParam);
}

//---------------------------------------------------------------------
