#include "pch.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

// カレントオブジェクトに関連するファイルパスを取得する。
void getPath(LPTSTR buffer)
{
	// カレントオブジェクトを取得する。
	int objectIndex = g_auin.GetCurrentObjectIndex();
	if (objectIndex < 0) return;
	ExEdit::Object* object = g_auin.GetObject(objectIndex);
	if (!object) return;

	// 先頭のフィルタを取得する。
	int filterIndex = 0;
	ExEdit::Filter* filter = g_auin.GetFilter(object, filterIndex);
	if (!filter) return;

	// 拡張データへのオフセットとサイズを取得する。
	int offset = -1, size = -1;
	int id = object->filter_param[filterIndex].id;
	switch (id)
	{
	case 0: offset = 0; size = 260; break; // 動画ファイル
	case 1: offset = 4; size = 256; break; // 画像ファイル
	case 2: offset = 0; size = 260; break; // 音声ファイル
	case 6: offset = 0; size = 260; break; // 音声波形表示
	case 82: offset = 0; size = 260; break; // 動画ファイル合成
	case 83: offset = 4; size = 256; break; // 画像ファイル合成
	}
	if (size <= 0) return; // サイズを取得できなかった場合は何もしない。

	// 拡張データからファイル名を取得する。
	BYTE* exdata = g_auin.GetExdata(object, filterIndex);
	LPCSTR fileName = (LPCSTR)(exdata + offset);

	// 整形してバッファに格納する。
	::StringCchPrintf(buffer, MAX_PATH, _T("%hs"), fileName);
	::PathRemoveFileSpec(buffer);
}

//---------------------------------------------------------------------

class ExplorerShuttle : public Shuttle
{
public:

	virtual LRESULT onTargetWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_NCDESTROY:
			{
				// ターゲットウィンドウが削除されたので、シャトルも削除する。

				MY_TRACE(_T("ColonyShuttle::onTargetWndProc(0x%08X, WM_NCDESTROY)\n"), hwnd);

				ShuttlePtr shuttle(shared_from_this());

				g_shuttleManager.removeShuttle(this);

				return Shuttle::onTargetWndProc(container, hwnd, message, wParam, lParam);
			}
		}

		return Shuttle::onTargetWndProc(container, hwnd, message, wParam, lParam);
	}
};

//---------------------------------------------------------------------

// エクスプローラを内包するウィンドウを作成する。
HWND createExplorer(LPCTSTR name)
{
	MY_TRACE(_T("createExplorer(%s)\n"), name);

	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.hCursor = ::LoadCursor(0, IDC_ARROW);
	wc.lpfnWndProc = explorerProc;
	wc.hInstance = g_instance;
	wc.lpszClassName = _T("SplitWindow.Explorer");
	::RegisterClass(&wc);

	if (!name)
	{
		for (int i = 0; TRUE; i++)
		{
			static TCHAR windowName[MAX_PATH] = {};
			::StringCbPrintf(windowName, sizeof(windowName), _T("Explorer.%d"), i);
			MY_TRACE_TSTR(windowName);

			if (g_shuttleManager.getShuttle(windowName))
				continue;

			name = windowName;

			break;
		}
	}

	HWND hwnd = ::CreateWindowEx(
		0,
		_T("SplitWindow.Explorer"),
		name,
		WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_THICKFRAME |
		WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		g_hub, 0, g_instance, 0);

	{
		// エクスプローラを内包するウィンドウをシャトルの中に入れる。
		ShuttlePtr shuttle = std::make_shared<ExplorerShuttle>();
		g_shuttleManager.addShuttle(shuttle, name, hwnd);
	}

	return hwnd;
}

//---------------------------------------------------------------------

// エクスプローラを内包するウィンドウのウィンドウ関数。
LRESULT CALLBACK explorerProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		{
			MY_TRACE(_T("explorerProc(WM_CREATE)\n"));

			g_explorerManager.insert(hwnd);

			break;
		}
	case WM_DESTROY:
		{
			MY_TRACE(_T("explorerProc(WM_DESTROY)\n"));

			g_explorerManager.sendExitMessage(hwnd);
			g_explorerManager.erase(hwnd);

			break;
		}
	case WM_CLOSE:
		{
			MY_TRACE(_T("explorerProc(WM_CLOSE)\n"));

			if (::GetKeyState(VK_SHIFT) < 0)
			{
				// Shift キーが押されている場合はエクスプローラを削除する。

				if (IDYES != ::MessageBox(hwnd, _T("エクスプローラを削除しますか？"), _T("SplitWindow"), MB_YESNO))
					return 0;

				// 何もしないとハブが他のウィンドウの後ろに隠れてしまうので、
				// 手動でフォアグラウンドにする。
				::SetActiveWindow(g_hub);
			}
			else
			{
				// エクスプローラの表示状態を切り替える。
				// ::ShowWindow(hwnd, SW_SHOW) ではなぜか WM_SHOWWINDOW が発生しないので、
				// 親ウィンドウの表示状態を直接切り替える。

				if (::IsWindowVisible(hwnd))
				{
					::ShowWindow(::GetParent(hwnd), SW_HIDE);
				}
				else
				{
					::ShowWindow(::GetParent(hwnd), SW_SHOW);
				}

				return 0;
			}
		
			break;
		}
	case WM_SIZE:
		{
			MY_TRACE(_T("explorerProc(WM_SIZE)\n"));

			ObjectExplorer* objectExplorer =
				g_explorerManager.getObjectExplorer(hwnd);

			::PostMessage(objectExplorer->dialog,
				WM_AVIUTL_OBJECT_EXPLORER_RESIZE, 0, 0);

			break;
		}
	}

	if (message == WM_AVIUTL_OBJECT_EXPLORER_INITED)
	{
		MY_TRACE(_T("explorerProc(WM_AVIUTL_OBJECT_EXPLORER_INITED)\n"));

		ObjectExplorer* objectExplorer =
			g_explorerManager.getObjectExplorer(hwnd);

		objectExplorer->dialog = (HWND)wParam;
		objectExplorer->browser = (HWND)lParam;

		MY_TRACE_HWND(objectExplorer->dialog);
		MY_TRACE_HWND(objectExplorer->browser);

		::PostMessage(objectExplorer->dialog,
			WM_AVIUTL_OBJECT_EXPLORER_RESIZE, 0, 0);
	}
	else if (message == WM_AVIUTL_OBJECT_EXPLORER_GET)
	{
		MY_TRACE(_T("explorerProc(WM_AVIUTL_OBJECT_EXPLORER_GET)\n"));

		TCHAR path[MAX_PATH] = {};
		getPath(path);
		if (_tcslen(path))
		{
			HWND dialog = (HWND)wParam;
			HWND url = (HWND)lParam;
			::SendMessage(url, WM_SETTEXT, 0, (LPARAM)path);
			::PostMessage(dialog, WM_COMMAND, IDOK, 0);
		}
	}

	return ::DefWindowProc(hwnd, message, wParam, lParam);
}

//---------------------------------------------------------------------
