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
int g_menuBreak = 30;
int g_tabMode = TabMode::bottom;
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

// ペインのレイアウトを再帰的に再計算する。
void calcLayout()
{
	MY_TRACE(_T("calcLayout()\n"));

	RECT rc; ::GetClientRect(g_singleWindow, &rc);

	g_root->recalcLayout(&rc);
}

void copyMenu(HMENU srcMenu, HMENU dstMenu)
{
	int c = ::GetMenuItemCount(srcMenu);

	for (int i = 0; i < c; i++)
	{
		TCHAR text[MAX_PATH] = {};
		MENUITEMINFO mii = { sizeof(mii) };
		mii.fMask = MIIM_BITMAP | MIIM_CHECKMARKS | MIIM_DATA | MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_STRING | MIIM_SUBMENU;
		mii.dwTypeData = text;
		mii.cch = MAX_PATH;
		::GetMenuItemInfo(srcMenu, i, TRUE, &mii);

		if (mii.hSubMenu)
		{
			HMENU subMenu = ::CreatePopupMenu();
			copyMenu(mii.hSubMenu, subMenu);
			mii.hSubMenu = subMenu;
		}

		::InsertMenuItem(dstMenu, i, TRUE, &mii);
	}
}

// ターゲットのメニューを表示する。
BOOL showTargetMenu(POINT point)
{
	// ペインを取得する。
	PanePtr pane = g_root->hitTestPane(point);
	if (!pane) return FALSE;

	Window* window = pane->getActiveWindow();
	if (!window) return FALSE;

	// 座標をチェックする。
	RECT rcMenu = pane->getMenuRect();
	if (!::PtInRect(&rcMenu, point)) return FALSE;

	// メニューを表示する座標を算出する。
	point.x = rcMenu.left;
	point.y = rcMenu.bottom;
	::ClientToScreen(g_singleWindow, &point);

	// ターゲットを取得する。
	HWND hwnd = window->m_hwnd;

	// ターゲットのメニューを取得する。
	HMENU srcMenu = ::GetMenu(hwnd);
	if (!srcMenu) return FALSE;

	// ポップアップメニューを作成する。
	HMENU dstMenu = ::CreatePopupMenu();
	copyMenu(srcMenu, dstMenu);

	// ポップアップメニューを表示する。
	::TrackPopupMenuEx(dstMenu, TPM_VERPOSANIMATION, point.x, point.y, hwnd, 0);

	// ポップアップメニューを削除する。
	::DestroyMenu(dstMenu);

	return TRUE;
}

// ペインの設定をするメニューを表示する。
void showPaneMenu()
{
	POINT cursorPos; ::GetCursorPos(&cursorPos);
	POINT point = cursorPos;
	::ScreenToClient(g_singleWindow, &point);

	PanePtr pane = g_root->hitTestPane(point);
	if (!pane) return;

	int c = pane->m_tab.getTabCount();
	int ht = (c <= 1) ? 0 : pane->m_tab.hitTest(point);

	HMENU menu = ::CreatePopupMenu();

	::AppendMenu(menu, MF_STRING, CommandID::SPLIT_MODE_NONE, _T("分割なし"));
	::AppendMenu(menu, MF_STRING, CommandID::SPLIT_MODE_VERT, _T("垂直線で分割"));
	::AppendMenu(menu, MF_STRING, CommandID::SPLIT_MODE_HORZ, _T("水平線で分割"));
	::AppendMenu(menu, MF_SEPARATOR, -1, 0);
	::AppendMenu(menu, MF_STRING, CommandID::ORIGIN_TOP_LEFT, _T("左上を原点にする"));
	::AppendMenu(menu, MF_STRING, CommandID::ORIGIN_BOTTOM_RIGHT, _T("右下を原点にする"));
	::AppendMenu(menu, MF_SEPARATOR, -1, 0);
	::AppendMenu(menu, MF_STRING, CommandID::MOVE_TO_LEFT, _T("左に移動する"));
	if (ht == -1 || ht <= 0)
		::EnableMenuItem(menu, CommandID::MOVE_TO_LEFT, MF_GRAYED | MF_DISABLED);
	::AppendMenu(menu, MF_STRING, CommandID::MOVE_TO_RIGHT, _T("右に移動する"));
	if (ht == -1 || ht >= c - 1)
		::EnableMenuItem(menu, CommandID::MOVE_TO_RIGHT, MF_GRAYED | MF_DISABLED);

	{
		::AppendMenu(menu, MF_STRING | MF_MENUBARBREAK, CommandID::WINDOW, _T("ドッキングを解除"));
		if (!pane->m_tab.getTabCount())
			::CheckMenuItem(menu, CommandID::WINDOW, MF_CHECKED);

		// 表示状態のウィンドウをメニューに追加する。
		int index = 1;
		for (auto& x : g_windowMap)
		{
			// 非表示状態のウィンドウはスキップする。
			if (!::IsWindowVisible(x.second->m_hwnd)) continue;

			// メニューアイテムを追加する。
			::AppendMenu(menu, MF_STRING, CommandID::WINDOW + index, x.first);

			// ウィンドウがタブの中に存在するなら
			if (pane->m_tab.findTab(x.second.get()) != -1)
			{
				// メニューアイテムにチェックを入れる。
				::CheckMenuItem(menu, CommandID::WINDOW + index, MF_CHECKED);
			}

			// カウンタをインクリメントする。
			index++;
		}

		// 非表示状態のウィンドウをメニューに追加する。
		int i = 0;
		for (auto& x : g_windowMap)
		{
			// 表示状態のウィンドウはスキップする。
			if (::IsWindowVisible(x.second->m_hwnd)) continue;

			UINT flags = MF_STRING;

			// 最初のメニューアイテムなら
			if (i == 0)
			{
				// 折り返しフラグを設定する。
				flags |= MF_MENUBARBREAK;
			}
			// 折り返し設定が有効なら
			else if (g_menuBreak)
			{
				// 折り返し位置かチェックする。
				if (i % g_menuBreak == 0)
				{
					// 折り返しフラグを設定する。
					flags |= MF_MENUBARBREAK;
				}
			}

			// メニューアイテムを追加する。
			::AppendMenu(menu, flags, CommandID::WINDOW + index, x.first);

			// ウィンドウがタブの中に存在するなら
			if (pane->m_tab.findTab(x.second.get()) != -1)
			{
				// メニューアイテムにチェックを入れる。
				::CheckMenuItem(menu, CommandID::WINDOW + index, MF_CHECKED);
			}

			// カウンタをインクリメントする。
			index++;
			i++;
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

	int id = ::TrackPopupMenu(menu, TPM_NONOTIFY | TPM_RETURNCMD, cursorPos.x, cursorPos.y, 0, g_singleWindow, 0);

	if (id)
	{
		switch (id)
		{
		case CommandID::SPLIT_MODE_NONE: pane->setSplitMode(SplitMode::none); break;
		case CommandID::SPLIT_MODE_VERT: pane->setSplitMode(SplitMode::vert); break;
		case CommandID::SPLIT_MODE_HORZ: pane->setSplitMode(SplitMode::horz); break;

		case CommandID::ORIGIN_TOP_LEFT: pane->m_origin = Origin::topLeft; break;
		case CommandID::ORIGIN_BOTTOM_RIGHT: pane->m_origin = Origin::bottomRight; break;

		case CommandID::MOVE_TO_LEFT: pane->m_tab.moveTab(ht, ht - 1); break;
		case CommandID::MOVE_TO_RIGHT: pane->m_tab.moveTab(ht, ht + 1); break;
		}

		if (id == CommandID::WINDOW)
		{
			if (ht != -1)
			{
				Window* window = pane->m_tab.getWindow(ht);
				pane->removeWindow(window);
			}
		}
		else if (id > CommandID::WINDOW)
		{
			TCHAR text[MAX_PATH] = {};
			::GetMenuString(menu, id, text, MAX_PATH, MF_BYCOMMAND);
			MY_TRACE_TSTR(text);

			auto it = g_windowMap.find(text);
			if (it != g_windowMap.end())
			{
				Window* window = it->second.get();
				Pane* oldPane = window->m_pane;

				int index = pane->addWindow(window, ht);
				if (index != -1)
				{
					if (oldPane)
						oldPane->recalcLayout();

					pane->m_tab.setCurrentIndex(index);
					pane->m_tab.changeCurrent();
				}
			}
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
LRESULT CALLBACK singleWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
			MY_TRACE(_T("singleWindowProc(WM_***KEY***, 0x%08X, 0x%08X, 0x%08X)\n"), message, wParam, lParam);

			return ::SendMessage(g_aviutlWindow->m_hwnd, message, wParam, lParam);
		}
	case WM_ACTIVATE: // 「patch.aul」用。
		{
			MY_TRACE(_T("singleWindowProc(WM_ACTIVATE, 0x%08X, 0x%08X)\n"), wParam, lParam);

			if (LOWORD(wParam) == WA_INACTIVE)
				::SendMessage(g_aviutlWindow->m_hwnd, message, wParam, lParam);

			break;
		}
	case WM_MENUSELECT: // 「patch.aul」用。
		{
			MY_TRACE(_T("singleWindowProc(WM_MENUSELECT, 0x%08X, 0x%08X)\n"), wParam, lParam);

			::SendMessage(g_aviutlWindow->m_hwnd, message, wParam, lParam);

			break;
		}
	case WM_CLOSE:
		{
			MY_TRACE(_T("singleWindowProc(WM_CLOSE, 0x%08X, 0x%08X)\n"), wParam, lParam);

			return ::SendMessage(g_aviutlWindow->m_hwnd, message, wParam, lParam);
		}
	case WM_COMMAND:
		{
			MY_TRACE(_T("singleWindowProc(WM_COMMAND, 0x%08X, 0x%08X)\n"), wParam, lParam);

			return ::SendMessage(g_aviutlWindow->m_hwnd, message, wParam, lParam);
		}
	case WM_SYSCOMMAND:
		{
			MY_TRACE(_T("singleWindowProc(WM_SYSCOMMAND, 0x%08X, 0x%08X)\n"), wParam, lParam);

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
			case SC_RESTORE:
			case SC_MINIMIZE: // 「patch.aul」用。
				{
					::SendMessage(g_aviutlWindow->m_hwnd, message, wParam, lParam);

					break;
				}
			}

			break;
		}
	case WM_NOTIFY:
		{
			NMHDR* header = (NMHDR*)lParam;

			switch (header->code)
			{
			case NM_RCLICK:
				{
					showPaneMenu();

					break;
				}
			case TCN_SELCHANGE:
				{
					Pane* pane = TabControl::getPane(header->hwndFrom);
					if (pane)
						pane->m_tab.changeCurrent();

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

			if (showTargetMenu(point))
				break;

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
				// マウス座標にあるペインを取得できたら
				PanePtr pane = g_root->hitTestPane(point);
				if (pane)
				{
					// クリックされたペインがウィンドウを持っているなら
					Window* window = pane->getActiveWindow();
					if (window)
						::SetFocus(window->m_hwnd); // そのウィンドウにフォーカスを当てる。
				}
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

			// レイアウト編集メニューを表示する。
			showPaneMenu();

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
