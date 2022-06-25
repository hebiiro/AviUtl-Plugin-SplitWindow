#include "pch.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

Container::Container(Window* window, DWORD style)
{
	m_window = window;
	m_hwnd = ::CreateWindowEx(
		0,
		_T("SplitWindow.Container"),
		_T("SplitWindow.Container"),
		style,
		100, 100, 200, 200,
		g_singleWindow, 0, g_instance, 0);

	// ウィンドウハンドルにポインタを関連付ける。
	setContainer(m_hwnd, this);

	// ポインタを関連付けてからウィンドウ関数が呼ばれるようにする。
	::SetWindowLong(m_hwnd, GWL_WNDPROC, (LONG)wndProc);
}

Container::~Container()
{
}

void Container::onResizeDockContainer(LPCRECT rc)
{
	// ドッキングコンテナのウィンドウ矩形を正規化された rc にする。

	int x = rc->left;
	int y = rc->top;
	int w = rc->right - rc->left;
	int h = rc->bottom - rc->top;

	x = min(rc->left, rc->right);
	y = min(rc->top, rc->bottom);
	w = max(w, 0);
	h = max(h, 0);

	forceSetWindowPos(m_hwnd, 0, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
}

void Container::onResizeFloatContainer()
{
	// ターゲットとコンテナのクライアント矩形が同じ位置になるように
	// フローティングコンテナのウィンドウ矩形を調整する。

	RECT rc; ::GetClientRect(m_window->m_hwnd, &rc);
	::MapWindowPoints(::GetParent(m_window->m_hwnd), 0, (LPPOINT)&rc, 2);
	clientToWindow(m_hwnd, &rc);
	int x = rc.left;
	int y = rc.top;
	int w = getWidth(rc);
	int h = getHeight(rc);

	forceSetWindowPos(m_hwnd, 0, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
}

BOOL Container::onSetContainerPos(WINDOWPOS* wp)
{
	// ターゲットとコンテナのクライアント矩形が同じ位置になるように
	// フローティングコンテナのウィンドウ矩形を調整する。

	RECT rc = { wp->x, wp->y, wp->x + wp->cx, wp->y + wp->cy }; // ターゲットのスクリーン座標のウィンドウ矩形。
	windowToClient(m_window->m_hwnd, &rc); // ターゲットのスクリーン座標のクライアント矩形。
	clientToWindow(m_hwnd, &rc); // コンテナのスクリーン座標のウィンドウ矩形。
	wp->x = rc.left;
	wp->y = rc.top;
	wp->cx = getWidth(rc);
	wp->cy = getHeight(rc);

	return TRUE;
}

LRESULT Container::onWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
		{
			// 背景色で塗りつぶす。

			PAINTSTRUCT ps = {};
			HDC dc = ::BeginPaint(hwnd, &ps);
			fillBackground(dc, &ps.rcPaint);
			::EndPaint(hwnd, &ps);
			return 0;
		}
	case WM_SIZE:
		{
			// ターゲットウィンドウの位置だけ変更する。サイズは変更しない。

			RECT rc = { 0, 0 };
			clientToWindow(m_window->m_hwnd, &rc);
			m_window->onSetTargetWindowPos(&rc);
			int x = rc.left;
			int y = rc.top;

			true_SetWindowPos(m_window->m_hwnd, 0,
				x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

			break;
		}
	case WM_SETFOCUS:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		{
			// ターゲットウィンドウにフォーカスを当てる。
			::SetFocus(m_window->m_hwnd);

			break;
		}
	case WM_COMMAND:
	case WM_CLOSE:
		{
			// メッセージをそのままターゲットウィンドウに転送する。
			return ::SendMessage(m_window->m_hwnd, message, wParam, lParam);
		}
	}

	return ::DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT Container::onTargetWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return ::CallWindowProc(m_window->m_targetWndProc, hwnd, message, wParam, lParam);
}

//---------------------------------------------------------------------

BOOL Container::registerWndClass()
{
	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.hCursor = ::LoadCursor(0, IDC_ARROW);
	wc.lpfnWndProc = ::DefWindowProc;
	wc.hInstance = g_instance;
	wc.lpszClassName = _T("SplitWindow.Container");
	return !!::RegisterClass(&wc);
}

LRESULT CALLBACK Container::wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Container* container = getContainer(hwnd);

	return container->m_window->onContainerWndProc(container, hwnd, message, wParam, lParam);
}

Container* Container::getContainer(HWND hwndContainer)
{
	return (Container*)::GetProp(hwndContainer, _T("SplitWindow.Container"));
}

void Container::setContainer(HWND hwndContainer, Container* container)
{
	::SetProp(hwndContainer, _T("SplitWindow.Container"), container);
}

//---------------------------------------------------------------------
