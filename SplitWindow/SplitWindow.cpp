#include "pch.h"
#include "SplitWindow.h"
#include "ConfigDialog.h"

//---------------------------------------------------------------------

// デバッグ用コールバック関数。デバッグメッセージを出力する
void ___outputLog(LPCTSTR text, LPCTSTR output)
{
	::OutputDebugString(output);
}

//---------------------------------------------------------------------

AviUtlInternal g_auin;
HINSTANCE g_instance = 0;
HWND g_singleWindow = 0;
HTHEME g_theme = 0;

AviUtlWindowPtr g_aviutlWindow(new AviUtlWindow());
ExEditWindowPtr g_exeditWindow(new ExEditWindow());
SettingDialogPtr g_settingDialog(new SettingDialog());

PanePtr g_root;
WindowMap g_windowMap;
PanePtr g_hotBorderPane;

int g_borderWidth = 8;
int g_captionHeight = 24;
int g_borderSnapRange = 8;
COLORREF g_fillColor = RGB(0x99, 0x99, 0x99);
COLORREF g_borderColor = RGB(0xcc, 0xcc, 0xcc);
COLORREF g_hotBorderColor = RGB(0x00, 0x00, 0x00);
COLORREF g_activeCaptionColor = ::GetSysColor(COLOR_HIGHLIGHT);
COLORREF g_activeCaptionTextColor = RGB(0xff, 0xff, 0xff);
COLORREF g_inactiveCaptionColor = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
COLORREF g_inactiveCaptionTextColor = RGB(0x00, 0x00, 0x00);
BOOL g_useTheme = FALSE;

//---------------------------------------------------------------------

// 他のウィンドウの土台となるシングルウィンドウを作成する。
HWND createSingleWindow()
{
	MY_TRACE(_T("createSingleWindow()\n"));

	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.hCursor = ::LoadCursor(0, IDC_ARROW);
	wc.lpfnWndProc = singleWindowProc;
	wc.hInstance = g_instance;
	wc.lpszClassName = _T("AviUtl"); // クラス名を AviUtl に偽装する。「AoiSupport」用。
	::RegisterClass(&wc);

	HWND hwnd = true_CreateWindowExA(
		0,
		"AviUtl",
		"SplitWindow",
		WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME |
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		0, 0, g_instance, 0);

	return hwnd;
}

// ペインのレイアウトを再帰的に再計算する。
void calcLayout()
{
	MY_TRACE(_T("calcLayout()\n"));

	RECT rc; ::GetClientRect(g_singleWindow, &rc);

	g_root->recalcLayout(&rc);
}

// ペインの設定をするメニューを表示する。
void showPaneMenu(POINT point)
{
	PanePtr pane = g_root->hitTestPane(point);
	if (!pane) return;

	BOOL allWindow = ::GetKeyState(VK_SHIFT) < 0;

	HMENU menu = ::CreatePopupMenu();

	::AppendMenu(menu, MF_STRING, CommandID::SPLIT_MODE_NONE, _T("分割なし"));
	::AppendMenu(menu, MF_STRING, CommandID::SPLIT_MODE_VERT, _T("垂直分割"));
	::AppendMenu(menu, MF_STRING, CommandID::SPLIT_MODE_HORZ, _T("水平分割"));
	::AppendMenu(menu, MF_SEPARATOR, -1, 0);
	::AppendMenu(menu, MF_STRING, CommandID::ORIGIN_TOP_LEFT, _T("左上を原点にする"));
	::AppendMenu(menu, MF_STRING, CommandID::ORIGIN_BOTTOM_RIGHT, _T("右下を原点にする"));
	::AppendMenu(menu, MF_SEPARATOR, -1, 0);

	{
		::AppendMenu(menu, MF_STRING, CommandID::WINDOW, _T("ウィンドウなし"));
		if (!pane->m_window)
			::CheckMenuItem(menu, CommandID::WINDOW, MF_CHECKED);

		int index = 1;
		for (auto& x : g_windowMap)
		{
			if (!allWindow)
				if (!::IsWindowVisible(x.second->m_hwnd)) continue;
			::AppendMenu(menu, MF_STRING, CommandID::WINDOW + index, x.first);
			if (pane->m_window == x.second)
				::CheckMenuItem(menu, CommandID::WINDOW + index, MF_CHECKED);
			index++;
		}
	}

	switch (pane->m_splitMode)
	{
	case SplitMode::none: ::CheckMenuItem(menu, CommandID::SPLIT_MODE_NONE, MF_CHECKED); break;
	case SplitMode::vert: ::CheckMenuItem(menu, CommandID::SPLIT_MODE_VERT, MF_CHECKED); break;
	case SplitMode::horz: ::CheckMenuItem(menu, CommandID::SPLIT_MODE_HORZ, MF_CHECKED); break;
	}

	switch (pane->m_origin)
	{
	case Origin::topLeft: ::CheckMenuItem(menu, CommandID::ORIGIN_TOP_LEFT, MF_CHECKED); break;
	case Origin::bottomRight: ::CheckMenuItem(menu, CommandID::ORIGIN_BOTTOM_RIGHT, MF_CHECKED); break;
	}

	POINT pt; ::GetCursorPos(&pt);
	int id = ::TrackPopupMenu(menu, TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, g_singleWindow, 0);

	if (id)
	{
		switch (id)
		{
		case CommandID::SPLIT_MODE_NONE: pane->setSplitMode(SplitMode::none); break;
		case CommandID::SPLIT_MODE_VERT: pane->setSplitMode(SplitMode::vert); break;
		case CommandID::SPLIT_MODE_HORZ: pane->setSplitMode(SplitMode::horz); break;

		case CommandID::ORIGIN_TOP_LEFT: pane->m_origin = Origin::topLeft; break;
		case CommandID::ORIGIN_BOTTOM_RIGHT: pane->m_origin = Origin::bottomRight; break;
		}

		if (id == CommandID::WINDOW)
		{
			pane->setWindow(0);
		}
		else if (id > CommandID::WINDOW)
		{
			TCHAR text[MAX_PATH] = {};
			::GetMenuString(menu, id, text, MAX_PATH, MF_BYCOMMAND);
			MY_TRACE_TSTR(text);

			auto it = g_windowMap.find(text);
			if (it != g_windowMap.end())
				pane->setWindow(it->second);
		}

		pane->recalcLayout();

		::InvalidateRect(g_singleWindow, 0, FALSE);
	}

	::DestroyMenu(menu);
}

void fillBackground(HDC dc, LPCRECT rc)
{
	// テーマを使用するなら
	if (g_useTheme)
	{
		int partId = WP_DIALOG;
		int stateId = 0;

		// テーマ API を使用してボーダーを描画する。
		::DrawThemeBackground(g_theme, dc, partId, stateId, rc, 0);
	}
	// テーマを使用しないなら
	else
	{
		// ブラシで塗りつぶす。
		HBRUSH brush = ::CreateSolidBrush(g_fillColor);
		::FillRect(dc, rc, brush);
		::DeleteObject(brush);
	}
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
	calcLayout();

	// 再描画する。
	::InvalidateRect(hwnd, 0, FALSE);

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
LRESULT CALLBACK singleWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SETFOCUS:
		{
			MY_TRACE(_T("singleWindowProc(WM_SETFOCUS)\n"));

			::SetFocus(g_aviutlWindow->m_hwnd);

			break;
		}
	case WM_COMMAND:
		{
			MY_TRACE(_T("singleWindowProc(WM_COMMAND)\n"));

			return ::SendMessage(g_aviutlWindow->m_hwnd, message, wParam, lParam);
		}
	case WM_SYSCOMMAND:
		{
			MY_TRACE(_T("singleWindowProc(WM_SYSCOMMAND)\n"));

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
			}

			break;
		}
	case WM_CREATE:
		{
			MY_TRACE(_T("singleWindowProc(WM_CREATE)\n"));

			g_theme = ::OpenThemeData(hwnd, VSCLASS_WINDOW);
			MY_TRACE_HEX(g_theme);

			HMENU menu = ::GetSystemMenu(hwnd, FALSE);
			::InsertMenu(menu, 0, MF_BYPOSITION | MF_STRING, CommandID::IMPORT_LAYOUT, _T("レイアウトのインポート"));
			::InsertMenu(menu, 1, MF_BYPOSITION | MF_STRING, CommandID::EXPORT_LAYOUT, _T("レイアウトのエクスポート"));
			::InsertMenu(menu, 2, MF_BYPOSITION | MF_STRING, CommandID::SHOW_CONFIG_DIALOG, _T("SplitWindowの設定"));
			::InsertMenu(menu, 3, MF_BYPOSITION | MF_SEPARATOR, 0, 0);

			break;
		}
	case WM_DESTROY:
		{
			MY_TRACE(_T("singleWindowProc(WM_DESTROY)\n"));

			::CloseThemeData(g_theme), g_theme = 0;

			break;
		}
	case WM_CLOSE:
		{
			MY_TRACE(_T("singleWindowProc(WM_CLOSE)\n"));

			return ::SendMessage(g_aviutlWindow->m_hwnd, message, wParam, lParam);
		}
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = ::BeginPaint(hwnd, &ps);
			RECT rc = ps.rcPaint;

			BP_PAINTPARAMS pp = { sizeof(pp) };
			HDC mdc = 0;
			HPAINTBUFFER pb = ::BeginBufferedPaint(dc, &rc, BPBF_COMPATIBLEBITMAP, &pp, &mdc);

			if (pb)
			{
				HDC dc = mdc;

				{
					// 背景を塗りつぶす。

					fillBackground(dc, &rc);
				}

				{
					// ボーダーを描画する。

					HBRUSH brush = ::CreateSolidBrush(g_borderColor);
					g_root->drawBorder(dc, brush);
					::DeleteObject(brush);
				}

				{
					// ホットボーダーを描画する。

					// ホットボーダーの矩形を取得できたら
					RECT rcHotBorder;
					if (g_hotBorderPane && g_hotBorderPane->getBorderRect(&rcHotBorder))
					{
						// テーマを使用するなら
						if (g_useTheme)
						{
							int partId = WP_BORDER;
							int stateId = CS_ACTIVE;

							// テーマ API を使用してボーダーを描画する。
							::DrawThemeBackground(g_theme, dc, partId, stateId, &rcHotBorder, 0);
						}
						// テーマを使用しないなら
						else
						{
							// ブラシで塗りつぶす。
							HBRUSH brush = ::CreateSolidBrush(g_hotBorderColor);
							::FillRect(dc, &rcHotBorder, brush);
							::DeleteObject(brush);
						}
					}
				}

				{
					// 各ウィンドウのキャプションを描画する。

					LOGFONTW lf = {};
					::GetThemeSysFont(g_theme, TMT_CAPTIONFONT, &lf);
					HFONT font = ::CreateFontIndirectW(&lf);
					HFONT oldFont = (HFONT)::SelectObject(dc, font);

					g_root->drawCaption(dc);

					::SelectObject(dc, oldFont);
					::DeleteObject(font);
				}

				::EndBufferedPaint(pb, TRUE);
			}

			::EndPaint(hwnd, &ps);
			return 0;
		}
	case WM_SIZE:
		{
			calcLayout();

			break;
		}
	case WM_SETCURSOR:
		{
			if (hwnd == (HWND)wParam)
			{
				POINT point; ::GetCursorPos(&point);
				::ScreenToClient(hwnd, &point);

				PanePtr borderPane = g_root->hitTestBorder(point);
				if (!borderPane) break;

				switch (borderPane->m_splitMode)
				{
				case SplitMode::vert:
					{
						::SetCursor(::LoadCursor(0, IDC_SIZEWE));

						return TRUE;
					}
				case SplitMode::horz:
					{
						::SetCursor(::LoadCursor(0, IDC_SIZENS));

						return TRUE;
					}
				}
			}

			break;
		}
	case WM_LBUTTONDOWN:
		{
			MY_TRACE(_T("singleWindowProc(WM_LBUTTONDOWN)\n"));

			// マウス座標を取得する。
			POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

			// マウス座標にあるボーダーを取得する。
			g_hotBorderPane = g_root->hitTestBorder(point);

			// ボーダーが有効かチェックする。
			if (g_hotBorderPane)
			{
				// オフセットを取得する。
				g_hotBorderPane->m_dragOffset = g_hotBorderPane->getDragOffset(point);

				// マウスキャプチャを開始する。
				::SetCapture(hwnd);

				// 再描画する。
				::InvalidateRect(hwnd, &g_hotBorderPane->m_position, FALSE);

				break;
			}

			{
				// マウス座標にあるペインを取得する。
				PanePtr pane = g_root->hitTestPane(point);

				// クリックされたペインがウィンドウを持っている場合はそのウィンドウにフォーカスを当てる。
				if (pane && pane->m_window)
					::SetFocus(pane->m_window->m_hwnd);
			}

			break;
		}
	case WM_LBUTTONUP:
		{
			MY_TRACE(_T("singleWindowProc(WM_LBUTTONUP)\n"));

			// マウス座標を取得する。
			POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

			// マウスをキャプチャ中かチェックする。
			if (::GetCapture() == hwnd)
			{
				// マウスキャプチャを終了する。
				::ReleaseCapture();

				if (g_hotBorderPane)
				{
					// ボーダーを動かす。
					g_hotBorderPane->dragBorder(point);

					// レイアウトを再計算する。
					g_hotBorderPane->recalcLayout();

					// 再描画する。
					::InvalidateRect(hwnd, &g_hotBorderPane->m_position, FALSE);
				}
			}

			break;
		}
	case WM_RBUTTONDOWN:
		{
			MY_TRACE(_T("singleWindowProc(WM_RBUTTONDOWN)\n"));

			// マウス座標を取得する。
			POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

			{
				// マウス座標にあるペインを取得する。
				PanePtr pane = g_root->hitTestPane(point);

				// クリックされたペインがウィンドウを持っている場合はそのウィンドウにフォーカスを当てる。
				if (pane && pane->m_window)
					::SetFocus(pane->m_window->m_hwnd);
			}

			break;
		}
	case WM_RBUTTONUP:
		{
			MY_TRACE(_T("singleWindowProc(WM_RBUTTONUP)\n"));

			// マウス座標を取得する。
			POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

			// レイアウト編集メニューを表示する。
			showPaneMenu(point);

			break;
		}
	case WM_MOUSEMOVE:
		{
			// マウス座標を取得する。
			POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

			// マウスをキャプチャ中かチェックする。
			if (::GetCapture() == hwnd)
			{
				if (g_hotBorderPane)
				{
					// ボーダーを動かす。
					g_hotBorderPane->dragBorder(point);

					// レイアウトを再計算する。
					g_hotBorderPane->recalcLayout();

					// 再描画する。
					::InvalidateRect(hwnd, &g_hotBorderPane->m_position, FALSE);
				}
			}
			else
			{
				// マウス座標にあるボーダーを取得する。
				PanePtr hotBorderPane = g_root->hitTestBorder(point);

				// 現在のホットボーダーと違う場合は
				if (g_hotBorderPane != hotBorderPane)
				{
					// ホットボーダーを更新する。
					g_hotBorderPane = hotBorderPane;

					// 再描画する。
					::InvalidateRect(hwnd, 0, FALSE);
				}

				// マウスリーブイベントをトラックする。
				TRACKMOUSEEVENT tme = { sizeof(tme) };
				tme.dwFlags = TME_LEAVE;
				tme.hwndTrack = hwnd;
				::TrackMouseEvent(&tme);
			}

			break;
		}
	case WM_MOUSELEAVE:
		{
			MY_TRACE(_T("singleWindowProc(WM_MOUSELEAVE)\n"));

			// 再描画用のペイン。
			PanePtr pane = g_hotBorderPane;

			// ホットボーダーが存在すれば
			if (g_hotBorderPane)
			{
				// ホットボーダーを無効にする。
				g_hotBorderPane.reset();

				// 再描画する。
				::InvalidateRect(hwnd, &pane->m_position, FALSE);
			}

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
			calcLayout();

			// シングルウィンドウをフォアグラウンドにする。
			::SetForegroundWindow(hwnd);
			::SetActiveWindow(hwnd);

			break;
		}
	}

	return ::DefWindowProc(hwnd, message, wParam, lParam);
}

//---------------------------------------------------------------------
