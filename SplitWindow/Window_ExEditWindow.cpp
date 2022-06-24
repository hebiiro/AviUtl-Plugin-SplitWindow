#include "pch.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

void ExEditWindow::init(HWND hwnd)
{
	Window::init(hwnd);

	// フローティングコンテナのアイコンを設定する。
	HICON icon = (HICON)::GetClassLong(m_hwnd, GCL_HICON);
	::SendMessage(m_floatContainer->m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
	::SendMessage(m_floatContainer->m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)icon);

	DWORD exedit = g_auin.GetExEdit();

	// スポイト処理の ::GetPixel() をフックする。
	hookAbsoluteCall(exedit + 0x22128, Dropper_GetPixel);

	{
		// キーボードフック処理の ::GetActiveWindow() をフックする。

		BYTE code[6];
		code[0] = (BYTE)0x90; // NOP
		code[1] = (BYTE)0xBD; // MOV EBP,DWORD
		*(DWORD*)&code[2] = (DWORD)KeyboardHook_GetActiveWindow;

		writeCode(exedit + 0x30D0E, code, sizeof(code));
	}
}

DWORD ExEditWindow::onGetTargetNewStyle()
{
	// ※ WS_CAPTION を外すとウィンドウの高さ調節にズレが生じる。
	DWORD style = ::GetWindowLong(m_hwnd, GWL_STYLE);
	style &= ~(WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
	return style;
}

void ExEditWindow::onSetTargetWindowPos(LPRECT rc)
{
	// ターゲットウィンドウのサイズを微調整する。
	rc->bottom -= g_auin.GetLayerHeight() / 2;
	::SendMessage(m_hwnd, WM_SIZING, WMSZ_BOTTOM, (LPARAM)rc);
}

LRESULT ExEditWindow::onContainerWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return Window::onContainerWndProc(container, hwnd, message, wParam, lParam);
}

LRESULT ExEditWindow::onTargetWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return Window::onTargetWndProc(container, hwnd, message, wParam, lParam);
}

//---------------------------------------------------------------------
