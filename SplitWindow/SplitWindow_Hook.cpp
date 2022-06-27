#include "pch.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

WNDPROC getClassProc(LPCWSTR className)
{
	WNDCLASSEXW wc = { sizeof(wc) };
	::GetClassInfoExW(0, className, &wc);
	return wc.lpfnWndProc;
}

void initHook()
{
	MY_TRACE(_T("initHook()\n"));

	true_ComboBoxProc = getClassProc(WC_COMBOBOXW);
	true_TrackBarProc = getClassProc(TRACKBAR_CLASSW);

	HMODULE user32 = ::GetModuleHandle(_T("user32.dll"));
	true_CreateWindowExA = (Type_CreateWindowExA)::GetProcAddress(user32, "CreateWindowExA");

	DetourTransactionBegin();
	DetourUpdateThread(::GetCurrentThread());

	ATTACH_HOOK_PROC(ComboBoxProc);
	ATTACH_HOOK_PROC(TrackBarProc);
	ATTACH_HOOK_PROC(CreateWindowExA);
	ATTACH_HOOK_PROC(MoveWindow);
	ATTACH_HOOK_PROC(SetWindowPos);
	ATTACH_HOOK_PROC(GetMenu);
	ATTACH_HOOK_PROC(SetMenu);
	ATTACH_HOOK_PROC(DrawMenuBar);

	ATTACH_HOOK_PROC(FindWindowA);
	ATTACH_HOOK_PROC(FindWindowW);
	ATTACH_HOOK_PROC(FindWindowExA);
	ATTACH_HOOK_PROC(GetWindow);
	ATTACH_HOOK_PROC(EnumThreadWindows);
	ATTACH_HOOK_PROC(EnumWindows);
	ATTACH_HOOK_PROC(SetWindowLongA);

	if (DetourTransactionCommit() == NO_ERROR)
	{
		MY_TRACE(_T("API フックに成功しました\n"));
	}
	else
	{
		MY_TRACE(_T("API フックに失敗しました\n"));
	}
}

void termHook()
{
	MY_TRACE(_T("termHook()\n"));
}

void addWindowToMap(WindowPtr window, LPCTSTR name, HWND hwnd)
{
	g_windowMap[name] = window;
	window->m_name = name;
	window->init(hwnd);
}

//---------------------------------------------------------------------

IMPLEMENT_HOOK_PROC_NULL(LRESULT, WINAPI, ComboBoxProc, (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam))
{
	switch (message)
	{
	case WM_MOUSEWHEEL:
		{
			return ::DefWindowProcW(hwnd, message, wParam, lParam);
		}
	}

	return true_ComboBoxProc(hwnd, message, wParam, lParam);
}

IMPLEMENT_HOOK_PROC_NULL(LRESULT, WINAPI, TrackBarProc, (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam))
{
	switch (message)
	{
	case WM_MOUSEWHEEL:
		{
			return ::DefWindowProcW(hwnd, message, wParam, lParam);
		}
	}

	return true_TrackBarProc(hwnd, message, wParam, lParam);
}

IMPLEMENT_HOOK_PROC_NULL(HWND, WINAPI, CreateWindowExA, (DWORD exStyle, LPCSTR className, LPCSTR windowName, DWORD style, int x, int y, int w, int h, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param))
{
	if ((DWORD)className <= 0x0000FFFF)
	{
		// className が ATOM の場合は何もしない。
		return true_CreateWindowExA(exStyle, className, windowName, style, x, y, w, h, parent, menu, instance, param);
	}

	// デバッグ用出力。
//	MY_TRACE(_T("CreateWindowExA(%hs, %hs)\n"), className, windowName);

	if (::lstrcmpiA(windowName, "AviUtl") == 0)
	{
		// AviUtl ウィンドウが作成される直前のタイミング。

		// ルートペインを作成する。
		g_root.reset(new Pane());

		// 土台となるシングルウィンドウを作成する。
		g_singleWindow = createSingleWindow();

		// コンテナのウィンドウクラスを登録する。
		Container::registerWndClass();
	}
	else if (::lstrcmpiA(className, "AviUtl") == 0 && parent && parent == g_aviutlWindow->m_hwnd)
	{
//		MY_TRACE_STR(windowName);

		// AviUtl のポップアップウィンドウの親をシングルウィンドウに変更する。
		parent = g_singleWindow;
	}

	HWND hwnd = true_CreateWindowExA(exStyle, className, windowName, style, x, y, w, h, parent, menu, instance, param);

	if (::lstrcmpiA(className, "AviUtl") == 0)
	{
		if (::lstrcmpiA(windowName, "AviUtl") == 0)
		{
			MY_TRACE_STR(windowName);

			// AviUtl ウィンドウのコンテナの初期化。
			addWindowToMap(g_aviutlWindow, _T("* AviUtl"), hwnd);
		}
		else if (::lstrcmpiA(windowName, "拡張編集") == 0)
		{
			MY_TRACE_STR(windowName);

			{
				// 拡張編集が読み込まれたのでアドレスを取得する。
				g_auin.initExEditAddress();

				DWORD exedit = g_auin.GetExEdit();

				// rikky_memory.auf + rikky_module.dll 用のフック。
				true_ScriptParamDlgProc = writeAbsoluteAddress(exedit + 0x3454 + 1, hook_ScriptParamDlgProc);
			}

			// 拡張編集ウィンドウのコンテナの初期化。
			addWindowToMap(g_exeditWindow, _T("* 拡張編集"), hwnd);
		}
		else if (parent && parent == g_singleWindow)
		{
			MY_TRACE_STR(windowName);

			// その他のプラグインウィンドウのコンテナの初期化。
			WindowPtr window(new Window());
			addWindowToMap(window, windowName, hwnd);
		}
	}
	else if (::lstrcmpiA(windowName, "ExtendedFilter") == 0)
	{
		// 設定ダイアログのコンテナの初期化。
		addWindowToMap(g_settingDialog, _T("* 設定ダイアログ"), hwnd);

		// すべてのウィンドウの初期化処理が終わったので
		// ポストメッセージ先で最初のレイアウト計算を行う。
		::PostMessage(g_singleWindow, WindowMessage::WM_POST_INIT, 0, 0);
	}

	return hwnd;
}

Window* getWindow(HWND hwnd)
{
	// WS_CHILD スタイルがあるかチェックする。
	DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
	if (!(style & WS_CHILD)) return 0;

	return Window::getWindow(hwnd);
}

IMPLEMENT_HOOK_PROC(BOOL, WINAPI, MoveWindow, (HWND hwnd, int x, int y, int w, int h, BOOL repaint))
{
	Window* window = getWindow(hwnd);

	if (!window) // Window を取得できない場合はデフォルト処理を行う。
		return true_MoveWindow(hwnd, x, y, w, h, repaint);

	MY_TRACE(_T("MoveWindow(0x%08X, %d, %d, %d, %d)\n"), hwnd, x, y, w, h);
	MY_TRACE_HWND(hwnd);
	MY_TRACE_WSTR((LPCWSTR)window->m_name);

	// ターゲットのウィンドウ位置を更新する。
	BOOL result = true_MoveWindow(hwnd, x, y, w, h, repaint);

	// 親ウィンドウを取得する。
	HWND parent = ::GetParent(hwnd);

	// 親ウィンドウがドッキングコンテナなら
	if (parent == window->m_dockContainer->m_hwnd)
	{
		// ドッキングコンテナにターゲットの新しいウィンドウ位置を算出させる。
		window->m_dockContainer->onWndProc(parent, WM_SIZE, 0, 0);
	}
	// 親ウィンドウがフローティングコンテナなら
	else if (parent == window->m_floatContainer->m_hwnd)
	{
		WINDOWPOS wp = {};
		wp.x = x; wp.y = y; wp.cx = w; wp.cy = h;

		// フローティングコンテナに自身の新しいウィンドウ位置を算出させる。
		if (window->m_floatContainer->onSetContainerPos(&wp))
		{
			// コンテナのウィンドウ位置を更新する。
			true_MoveWindow(parent, wp.x, wp.y, wp.cx, wp.cy, repaint);
		}
	}

	return result;
}

IMPLEMENT_HOOK_PROC(BOOL, WINAPI, SetWindowPos, (HWND hwnd, HWND insertAfter, int x, int y, int w, int h, UINT flags))
{
	Window* window = getWindow(hwnd);

	if (!window) // Window を取得できない場合はデフォルト処理を行う。
		return true_SetWindowPos(hwnd, insertAfter, x, y, w, h, flags);

	MY_TRACE(_T("SetWindowPos(0x%08X, %d, %d, %d, %d)\n"), hwnd, x, y, w, h);
	MY_TRACE_HWND(hwnd);
	MY_TRACE_WSTR((LPCWSTR)window->m_name);

	// ターゲットのウィンドウ位置を更新する。
	BOOL result = true_SetWindowPos(hwnd, insertAfter, x, y, w, h, flags);

	// 親ウィンドウを取得する。
	HWND parent = ::GetParent(hwnd);

	// 親ウィンドウがドッキングコンテナなら
	if (parent == window->m_dockContainer->m_hwnd)
	{
		// ドッキングコンテナにターゲットの新しいウィンドウ位置を算出させる。
		window->m_dockContainer->onWndProc(parent, WM_SIZE, 0, 0);
	}
	// 親ウィンドウがフローティングコンテナなら
	else if (parent == window->m_floatContainer->m_hwnd)
	{
		WINDOWPOS wp = {};
		wp.x = x; wp.y = y; wp.cx = w; wp.cy = h;

		// フローティングコンテナに自身の新しいウィンドウ位置を算出させる。
		if (window->m_floatContainer->onSetContainerPos(&wp))
		{
			// コンテナのウィンドウ位置を更新する。
			true_SetWindowPos(parent, insertAfter, wp.x, wp.y, wp.cx, wp.cy, flags);
		}
	}

	return result;
}

/*
	GetMenu、SetMenu、DrawMenuBar では
	AviUtl ウィンドウのハンドルが渡されたとき、シングルウィンドウのハンドルに取り替えて偽装する。
	これによって、AviUtl ウィンドウのメニュー処理がシングルウィンドウに対して行われるようになる。
*/
HWND getMenuOwner(HWND hwnd)
{
	if (hwnd == g_aviutlWindow->m_hwnd)
	{
		hwnd = g_singleWindow;
	}
	else
	{
		Window* window = Window::getWindow(hwnd);
		if (window)
			hwnd = window->m_floatContainer->m_hwnd;
	}

	return hwnd;
}

IMPLEMENT_HOOK_PROC(HMENU, WINAPI, GetMenu, (HWND hwnd))
{
//	MY_TRACE(_T("GetMenu(0x%08X)\n"), hwnd);

	hwnd = getMenuOwner(hwnd);

	return true_GetMenu(hwnd);
}

IMPLEMENT_HOOK_PROC(BOOL, WINAPI, SetMenu, (HWND hwnd, HMENU menu))
{
//	MY_TRACE(_T("SetMenu(0x%08X, 0x%08X)\n"), hwnd, menu);

	hwnd = getMenuOwner(hwnd);

	return true_SetMenu(hwnd, menu);
}

IMPLEMENT_HOOK_PROC(BOOL, WINAPI, DrawMenuBar, (HWND hwnd))
{
//	MY_TRACE(_T("DrawMenuBar(0x%08X)\n"), hwnd);

	hwnd = getMenuOwner(hwnd);

	return true_DrawMenuBar(hwnd);
}

IMPLEMENT_HOOK_PROC(HWND, WINAPI, FindWindowA, (LPCSTR className, LPCSTR windowName))
{
	MY_TRACE(_T("FindWindowA(%hs, %hs)\n"), className, windowName);

	// 「ショートカット再生」用。
	if (className && windowName && ::lstrcmpiA(className, "AviUtl") == 0)
	{
		auto it = g_windowMap.find(windowName);
		if (it != g_windowMap.end())
			return it->second->m_hwnd;
	}

	return true_FindWindowA(className, windowName);
}

IMPLEMENT_HOOK_PROC(HWND, WINAPI, FindWindowW, (LPCWSTR className, LPCWSTR windowName))
{
	MY_TRACE(_T("FindWindowW(%ws, %ws)\n"), className, windowName);

	// 「PSDToolKit」の「送る」用。
	if (className && ::lstrcmpiW(className, L"ExtendedFilterClass") == 0)
		return g_settingDialog->m_hwnd;

	return true_FindWindowW(className, windowName);
}

IMPLEMENT_HOOK_PROC(HWND, WINAPI, FindWindowExA, (HWND parent, HWND childAfter, LPCSTR className, LPCSTR windowName))
{
	MY_TRACE(_T("FindWindowExA(0x%08X, 0x%08X, %hs, %hs)\n"), parent, childAfter, className, windowName);

	// 「テキスト編集補助プラグイン」用。
	if (!parent && className && ::lstrcmpiA(className, "ExtendedFilterClass") == 0)
		return g_settingDialog->m_hwnd;

	return true_FindWindowExA(parent, childAfter, className, windowName);
}

IMPLEMENT_HOOK_PROC(HWND, WINAPI, GetWindow, (HWND hwnd, UINT cmd))
{
//	MY_TRACE(_T("GetWindow(0x%08X, %d)\n"), hwnd, cmd);
//	MY_TRACE_HWND(hwnd);

	if (cmd == GW_OWNER)
	{
		if (hwnd == g_exeditWindow->m_hwnd)
		{
			// 拡張編集ウィンドウのオーナーウィンドウは AviUtl ウィンドウ。
			return g_aviutlWindow->m_hwnd;
		}
		else if (hwnd == g_settingDialog->m_hwnd)
		{
			// 設定ダイアログのオーナーウィンドウは拡張編集ウィンドウ。
			return g_exeditWindow->m_hwnd;
		}

		HWND retValue = true_GetWindow(hwnd, cmd);

		if (retValue == g_singleWindow)
		{
			// 「スクリプト並べ替え管理」「シークバー＋」などの一般的なプラグイン用。
			// シングルウィンドウがオーナーになっている場合は AviUtl ウィンドウを返すようにする。
			return g_aviutlWindow->m_hwnd;
		}

		return retValue;
	}

	return true_GetWindow(hwnd, cmd);
}

IMPLEMENT_HOOK_PROC(BOOL, WINAPI, EnumThreadWindows, (DWORD threadId, WNDENUMPROC enumProc, LPARAM lParam))
{
	MY_TRACE(_T("EnumThreadWindows(%d, 0x%08X, 0x%08X)\n"), threadId, enumProc, lParam);

	// 「イージング設定時短プラグイン」用。
	if (threadId == ::GetCurrentThreadId() && enumProc && lParam)
	{
		// enumProc() の中で ::GetWindow() が呼ばれる。
		if (!enumProc(g_settingDialog->m_hwnd, lParam))
			return FALSE;
	}

	return true_EnumThreadWindows(threadId, enumProc, lParam);
}

IMPLEMENT_HOOK_PROC(BOOL, WINAPI, EnumWindows, (WNDENUMPROC enumProc, LPARAM lParam))
{
	MY_TRACE(_T("EnumWindows(0x%08X, 0x%08X)\n"), enumProc, lParam);

	// 「拡張編集RAMプレビュー」用。
	if (enumProc && lParam)
	{
		if (!enumProc(g_aviutlWindow->m_hwnd, lParam))
			return FALSE;
	}

	return true_EnumWindows(enumProc, lParam);
}

IMPLEMENT_HOOK_PROC(LONG, WINAPI, SetWindowLongA, (HWND hwnd, int index, LONG newLong))
{
//	MY_TRACE(_T("SetWindowLongA(0x%08X, %d, 0x%08X)\n"), hwnd, index, newLong);

	// 「拡張ツールバー」用。
	if (index == GWL_HWNDPARENT)
	{
		Window* window = Window::getWindow(hwnd);

		if (window)
		{
			MY_TRACE(_T("Window が取得できるウィンドウは HWNDPARENT を変更できません\n"));
			return 0;
		}
	}

	return true_SetWindowLongA(hwnd, index, newLong);
}

IMPLEMENT_HOOK_PROC_NULL(INT_PTR, CALLBACK, ScriptParamDlgProc, (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam))
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			MY_TRACE(_T("ScriptParamDlgProc(WM_INITDIALOG)\n"));

			// rikky_memory.auf + rikky_module.dll 用。
			::PostMessage(g_settingDialog->m_hwnd, WM_NCACTIVATE, FALSE, (LPARAM)hwnd);

			break;
		}
	}

	return true_ScriptParamDlgProc(hwnd, message, wParam, lParam);
}

COLORREF WINAPI Dropper_GetPixel(HDC _dc, int x, int y)
{
	MY_TRACE(_T("Dropper_GetPixel(0x%08X, %d, %d)\n"), _dc, x, y);

	// すべてのモニタのすべての場所から色を抽出できるようにする。

	POINT point; ::GetCursorPos(&point);
	::LogicalToPhysicalPointForPerMonitorDPI(0, &point);
	HDC dc = ::GetDC(0);
	COLORREF color = ::GetPixel(dc, point.x, point.y);
	::ReleaseDC(0, dc);
	return color;
}

// hwnd が child の先祖かどうか調べる。
BOOL isAncestor(HWND hwnd, HWND child)
{
	while (child)
	{
		if (child == hwnd)
			return TRUE;

		child = ::GetParent(child);
	}

	return FALSE;
}

HWND WINAPI KeyboardHook_GetActiveWindow()
{
	MY_TRACE(_T("KeyboardHook_GetActiveWindow()\n"));

	HWND focus = ::GetFocus();

	if (isAncestor(g_settingDialog->m_hwnd, focus))
	{
		MY_TRACE(_T("設定ダイアログを返します\n"));
		return g_settingDialog->m_hwnd;
	}

	if (isAncestor(g_exeditWindow->m_hwnd, focus))
	{
		MY_TRACE(_T("拡張編集ウィンドウを返します\n"));
		return g_exeditWindow->m_hwnd;
	}

	return ::GetActiveWindow();
}

//---------------------------------------------------------------------

EXTERN_C BOOL APIENTRY DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		{
			// ロケールを設定する。
			// これをやらないと日本語テキストが文字化けするので最初に実行する。
			_tsetlocale(LC_ALL, _T(""));

			MY_TRACE(_T("DLL_PROCESS_ATTACH\n"));

			// この DLL のハンドルをグローバル変数に保存しておく。
			g_instance = instance;
			MY_TRACE_HEX(g_instance);

			// この DLL の参照カウンタを増やしておく。
			WCHAR moduleFileName[MAX_PATH] = {};
			::GetModuleFileNameW(g_instance, moduleFileName, MAX_PATH);
			::LoadLibraryW(moduleFileName);

			initHook();

			break;
		}
	case DLL_PROCESS_DETACH:
		{
			MY_TRACE(_T("DLL_PROCESS_DETACH\n"));

			termHook();

			break;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------
