#include "pch.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

SpreadContainer::SpreadContainer(Window* window, DWORD style)
	: Container(window, style)
{
}

SpreadContainer::~SpreadContainer()
{
}

LRESULT SpreadContainer::onWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		{
			// ターゲットウィンドウをコンテナウィンドウのクライアント領域全体まで広げる。

			RECT rc; ::GetClientRect(hwnd, &rc);
			clientToWindow(m_window->m_hwnd, &rc);
			m_window->onSetTargetWindowPos(&rc);
			int x = rc.left;
			int y = rc.top;
			int w = rc.right - rc.left;
			int h = rc.bottom - rc.top;

			true_SetWindowPos(m_window->m_hwnd,
				0, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);

			return 0;
		}
	}

	return Container::onWndProc(hwnd, message, wParam, lParam);
}

//---------------------------------------------------------------------
