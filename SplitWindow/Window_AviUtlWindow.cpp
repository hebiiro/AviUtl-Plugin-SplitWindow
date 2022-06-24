#include "pch.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

void AviUtlWindow::init(HWND hwnd)
{
	Window::init(hwnd);

	// フローティングコンテナのアイコンを設定する。
	HICON icon = (HICON)::GetClassLong(m_hwnd, GCL_HICON);
	::SendMessage(m_floatContainer->m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
	::SendMessage(m_floatContainer->m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)icon);

	// シングルウィンドウのアイコンを設定する。
	::SetClassLong(g_singleWindow, GCL_HICON, (LONG)icon);
	::SetClassLong(g_singleWindow, GCL_HICONSM, (LONG)icon);
}

LRESULT AviUtlWindow::onContainerWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return Window::onContainerWndProc(container, hwnd, message, wParam, lParam);
}

LRESULT AviUtlWindow::onTargetWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
		{
			MY_TRACE(_T("AviUtlWindow::onTargetWndProc(WM_CLOSE)\n"));

			// AviUtl が終了しようとしているので設定を保存する。
			saveConfig();

			break;
		}
	case WM_SETTEXT:
		{
//			MY_TRACE(_T("AviUtlWindow::onTargetWndProc(WM_SETTEXT)\n"));

			// コンテナのウィンドウテキストを更新する。
			::SetWindowText(::GetParent(hwnd), (LPCTSTR)lParam);

			::InvalidateRect(g_singleWindow, 0, FALSE);

			// シングルウィンドウのウィンドウテキストを更新する。

			AviUtl::EditHandle* editp = (AviUtl::EditHandle*)g_auin.GetEditp();

			char fileName[MAX_PATH] = {};
			if (editp->frame_n)
			{
				::StringCbCopyA(fileName, sizeof(fileName), editp->project_filename);
				::PathStripPathA(fileName);

				if (::lstrlenA(fileName) == 0)
					::StringCbCopyA(fileName, sizeof(fileName), (LPCSTR)lParam);
			}
			else
			{
				::StringCbCopyA(fileName, sizeof(fileName), "無題");
			}
			::StringCbCatA(fileName, sizeof(fileName), " - AviUtl");
			::SetWindowTextA(g_singleWindow, fileName);

			return container->onTargetWndProc(hwnd, message, wParam, lParam);
		}
	}

	return Window::onTargetWndProc(container, hwnd, message, wParam, lParam);
}

//---------------------------------------------------------------------
