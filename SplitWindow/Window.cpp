#include "pch.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

void Window::init(HWND hwnd)
{
	// ターゲットのウィンドウハンドルを格納する。
	m_hwnd = hwnd;

	// ウィンドウハンドルにポインタを結び付ける。
	setWindow(m_hwnd, this);

	// コンテナを作成する。
	m_dockContainer.reset(onCreateDockContainer());
	m_floatContainer.reset(onCreateFloatContainer());

	// ターゲットのウィンドウスタイルを変更する。
	DWORD style = onGetTargetNewStyle();
	::SetWindowLong(m_hwnd, GWL_STYLE, style | WS_CHILD);
//	::SetWindowPos(m_hwnd, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);

	// ターゲットのウィンドウタイトルを取得する。
	TCHAR text[MAX_PATH] = {};
	::GetWindowText(m_hwnd, text, MAX_PATH);
	::SetWindowText(m_floatContainer->m_hwnd, text);

	// ターゲットウィンドウのウィンドウ矩形を取得しておく。
	RECT rc; ::GetWindowRect(m_hwnd, &rc);
	int x = rc.left;
	int y = rc.top;
	int w = getWidth(rc);
	int h = getHeight(rc);

	// ターゲットの親をフローティングコンテナに変更する。
	::SetParent(m_hwnd, m_floatContainer->m_hwnd);
	::SendMessage(m_hwnd, WM_UPDATEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS), 0);

	// ::SetWindowPos() を呼び出し、フック処理を促す。
	// コンテナの種類によってはコンテナのウィンドウサイズが調整される。
	::SetWindowPos(m_hwnd, 0, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

	// ターゲットが表示状態ならフローティングコンテナを表示する。
	if (::IsWindowVisible(m_hwnd))
		::ShowWindow(m_floatContainer->m_hwnd, SW_SHOW);

	// ターゲットをサブクラス化する。
	m_targetWndProc = (WNDPROC)::SetWindowLong(m_hwnd, GWL_WNDPROC, (LONG)targetWndProc);
}

Container* Window::onCreateDockContainer()
{
	// ドッキングコンテナのスタイルを決定する。
	DWORD style = ::GetWindowLong(m_hwnd, GWL_STYLE);
	DWORD dockStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	// ドッキングコンテナのタイプを決定する。
	if (style & WS_THICKFRAME)
		return new SpreadContainer(this, dockStyle);
	else
		return new ScrollContainer(this, dockStyle);
}

Container* Window::onCreateFloatContainer()
{
	// フローティングコンテナのスタイルを決定する。
	DWORD style = ::GetWindowLong(m_hwnd, GWL_STYLE);
	DWORD floatStyle = WS_CAPTION | WS_SYSMENU | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	floatStyle |= style & (WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

	// フローティングコンテナのタイプを決定する。
	if (style & WS_THICKFRAME)
		return new SpreadContainer(this, floatStyle);
	else
		return new Container(this, floatStyle);
}

DWORD Window::onGetTargetNewStyle()
{
	// ※ WS_CAPTION を外さないとエディットボックスでマウス処理が行われなくなる。
	DWORD style = ::GetWindowLong(m_hwnd, GWL_STYLE);
	style &= ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
	return style;
}

void Window::onSetTargetWindowPos(LPRECT rc)
{
	// デフォルトでは何もしない。
}

LRESULT Window::onContainerWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return container->onWndProc(hwnd, message, wParam, lParam);
}

LRESULT Window::onTargetWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_NCPAINT:
		{
			// クライアント領域を除外してから塗り潰す。
			HDC dc = ::GetWindowDC(hwnd);
			RECT rc; ::GetWindowRect(hwnd, &rc);
			RECT rcClip; ::GetClientRect(hwnd, &rcClip);
			::MapWindowPoints(hwnd, 0, (LPPOINT)&rcClip, 2);
			::OffsetRect(&rcClip, -rc.left, -rc.top);
			::OffsetRect(&rc, -rc.left, -rc.top);
			HRGN rgn = ::CreateRectRgnIndirect(&rcClip);
			::ExtSelectClipRgn(dc, rgn, RGN_DIFF);
			::DeleteObject(rgn);
			HBRUSH brush = ::CreateSolidBrush(g_fillColor);
			::FillRect(dc, &rc, brush);
			::DeleteObject(brush);
			::ReleaseDC(hwnd, dc);
			return 0;
		}
	case WM_SHOWWINDOW:
		{
			MY_TRACE(_T("WM_SHOWWINDOW, 0x%08X, 0x%08X\n"), wParam, lParam);

			// ターゲットウィンドウの表示状態が変更されたらコンテナもそれに追従する。
			::ShowWindow(::GetParent(hwnd), wParam ? SW_SHOW : SW_HIDE);

			break;
		}
	case WM_SETTEXT:
		{
			// コンテナのウィンドウテキストを更新する。
			::SetWindowText(::GetParent(hwnd), (LPCTSTR)lParam);

			break;
		}
	case WM_SETFOCUS:
	case WM_KILLFOCUS:
		{
			// ペインのタイトル部分を再描画する。
			::InvalidateRect(g_singleWindow, 0, FALSE);

			break;
		}
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		{
			// ターゲットウィンドウがクリックされたらフォーカスを当てるようにする。
			::SetFocus(hwnd);

			break;
		}
	case WM_GETMINMAXINFO:
		{
			// ウィンドウの限界サイズを増やしておく。
			MINMAXINFO* mmi = (MINMAXINFO*)lParam;
			mmi->ptMaxTrackSize.y *= 3;

			break;
		}
	}

	return container->onTargetWndProc(hwnd, message, wParam, lParam);
}

void Window::dockWindow(LPCRECT rc)
{
	resizeDockContainer(rc);

	// 親ウィンドウをドッキングコンテナに切り替える。
	::SetParent(m_hwnd, m_dockContainer->m_hwnd);
	::ShowWindow(m_floatContainer->m_hwnd, SW_HIDE);
	::ShowWindow(m_dockContainer->m_hwnd, SW_SHOW);
}

void Window::floatWindow()
{
	TCHAR text[MAX_PATH] = {};
	::GetWindowText(m_hwnd, text, MAX_PATH);
	::SetWindowText(m_floatContainer->m_hwnd, text);

	resizeFloatContainer();

	// 親ウィンドウをフローティングコンテナに切り替える。
	::SetParent(m_hwnd, m_floatContainer->m_hwnd);
	::ShowWindow(m_dockContainer->m_hwnd, SW_HIDE);
	if (::IsWindowVisible(m_hwnd))
		::ShowWindow(m_floatContainer->m_hwnd, SW_SHOW);
}

void Window::resizeDockContainer(LPCRECT rc)
{
	// 矩形からペインのタイトル部分を除外する。
	RECT rc2 = *rc;
	rc2.top += g_captionHeight;
	m_dockContainer->onResizeDockContainer(&rc2);
}

void Window::resizeFloatContainer()
{
	m_floatContainer->onResizeFloatContainer();
}

LRESULT CALLBACK Window::targetWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hwndContainer = ::GetParent(hwnd);
	Container* container = Container::getContainer(hwndContainer);
	return container->m_window->onTargetWndProc(container, hwnd, message, wParam, lParam);
}

Window* Window::getWindow(HWND hwnd)
{
	return (Window*)::GetProp(hwnd, _T("SplitWindow.Window"));
}

void Window::setWindow(HWND hwnd, Window* window)
{
	::SetProp(hwnd, _T("SplitWindow.Window"), window);
}

//---------------------------------------------------------------------
