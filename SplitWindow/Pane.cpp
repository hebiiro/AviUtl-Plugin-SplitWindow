﻿#include "pch.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

TabControl::TabControl(Pane* pane)
{
	m_hwnd = ::CreateWindowEx(
		0,
		WC_TABCONTROL,
		_T("SplitWindow.Tab"),
		WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		TCS_FOCUSNEVER,
		0, 0, 0, 0,
		g_singleWindow, 0, g_instance, 0);

	setPane(m_hwnd, pane);

	SYS_INFO si = {};
	g_auin.get_sys_info(0, &si);
	::SendMessage(m_hwnd, WM_SETFONT, (WPARAM)si.hfont, TRUE);
}

TabControl::~TabControl()
{
	::DestroyWindow(m_hwnd);
}

Pane* TabControl::getPane(HWND hwnd)
{
	return (Pane*)::GetProp(hwnd, _T("SplitWindow.Pane"));
}

void TabControl::setPane(HWND hwnd, Pane* pane)
{
	::SetProp(hwnd, _T("SplitWindow.Pane"), pane);
}

int TabControl::getTabCount()
{
	return ::SendMessage(m_hwnd, TCM_GETITEMCOUNT, 0, 0);
}

Window* TabControl::getWindow(int index)
{
	if (index == -1) return 0;

	TCITEM item = {};
	item.mask = TCIF_PARAM;
	::SendMessage(m_hwnd, TCM_GETITEM, index, (LPARAM)&item);

	return (Window*)item.lParam;
}

int TabControl::getCurrentIndex()
{
	return ::SendMessage(m_hwnd, TCM_GETCURSEL, 0, 0);
}

int TabControl::setCurrentIndex(int index)
{
	if (index < 0 || index >= getTabCount())
		index = 0;

	return ::SendMessage(m_hwnd, TCM_SETCURSEL, index, 0);
}

int TabControl::hitTest(POINT point)
{
	TCHITTESTINFO hti = {};
	hti.pt = point;
	::MapWindowPoints(g_singleWindow, m_hwnd, &hti.pt, 1);
	return ::SendMessage(m_hwnd, TCM_HITTEST, 0, (LPARAM)&hti);
}

int TabControl::addTab(Window* window, LPCTSTR text, int index)
{
	TCITEM item = {};
	item.mask = TCIF_PARAM | TCIF_TEXT;
	item.lParam = (LPARAM)window;
	item.pszText = (LPTSTR)text;
	return ::SendMessage(m_hwnd, TCM_INSERTITEM, index, (LPARAM)&item);
}

void TabControl::deleteTab(int index)
{
	::SendMessage(m_hwnd, TCM_DELETEITEM, index, 0);
}

void TabControl::deleteAllTabs()
{
	::SendMessage(m_hwnd, TCM_DELETEALLITEMS, 0, 0);
}

int TabControl::findTab(Window* window)
{
	int c = getTabCount();
	for (int i = 0; i < c; i++)
	{
		if (getWindow(i) == window)
			return i;
	}

	return -1;
}

int TabControl::moveTab(int from, int to)
{
	int c = getTabCount();

	if (from < 0 || from >= c) return -1;
	if (to < 0 || to >= c) return -1;

	int current = getCurrentIndex();
	TCHAR text[MAX_PATH] = {};
	TCITEM item = {};
	item.mask = TCIF_PARAM | TCIF_TEXT;
	item.pszText = text;
	item.cchTextMax = MAX_PATH;
	::SendMessage(m_hwnd, TCM_GETITEM, from, (LPARAM)&item);
	::SendMessage(m_hwnd, TCM_DELETEITEM, from, 0);
	::SendMessage(m_hwnd, TCM_INSERTITEM, to, (LPARAM)&item);
	if (from == current) setCurrentIndex(to);

	return to;
}

void TabControl::changeCurrent()
{
	int current = getCurrentIndex();
	int c = getTabCount();
	for (int i = 0; i < c; i++)
	{
		Window* window = getWindow(i);

		::ShowWindow(window->m_dockContainer->m_hwnd, (i == current) ? SW_SHOW : SW_HIDE);
	}

	::InvalidateRect(g_singleWindow, &getPane(m_hwnd)->m_position, FALSE);
}

void TabControl::changeText(Window* window, LPCTSTR text)
{
	int index = findTab(window);
	if (index == -1) return;

	TCITEM item = {};
	item.mask = TCIF_TEXT;
	item.pszText = (LPTSTR)text;
	::SendMessage(m_hwnd, TCM_SETITEM, index, (LPARAM)&item);
}

//---------------------------------------------------------------------

Pane::Pane()
	: m_tab(this)
{
}

Pane::~Pane()
{
}

Window* Pane::getActiveWindow()
{
	int current = m_tab.getCurrentIndex();
	return m_tab.getWindow(current);
}

int Pane::addWindow(Window* window, int index)
{
	MY_TRACE(_T("Pane::addWindow(0x%08X, %d)\n"), window, index);

	// ウィンドウはすでにドッキング済みなので何もしない。
	if (m_tab.findTab(window) != -1) return -1;

	// ウィンドウが他のペインとドッキング済みなら
	if (window->m_pane)
	{
		// 他のペインとのドッキングを解除させる。
		window->m_pane->removeWindow(window);
	}

	// 追加位置が無効の場合は末尾に追加する。
	if (index == -1)
		index = m_tab.getTabCount();

	// ウィンドウテキストを取得する。
	TCHAR text[MAX_PATH] = {};
	::GetWindowText(window->m_hwnd, text, MAX_PATH);

	// タブを追加する。
	int result = m_tab.addTab(window, text, index);

	// ウィンドウをドッキング状態にする。
	window->m_pane = this;
	window->dockWindow(&m_position);

	return result;
}

void Pane::removeWindow(Window* window)
{
	MY_TRACE(_T("Pane::removeWindow(0x%08X)\n"), window);

	// ウィンドウ側からの参照を削除する。
	window->m_pane = 0;

	// ペイン側からの参照を削除する。
	int index = m_tab.findTab(window);
	if (index != -1)
	{
		BOOL same = index == m_tab.getCurrentIndex();

		// タブを削除する。
		m_tab.deleteTab(index);

		// 削除されたアイテムとカレントアイテムが同じなら
		if (same)
		{
			// カレントアイテムを設定し直す。
			m_tab.setCurrentIndex(index);
		}

		// カレントアイテムが無効なら
		if (m_tab.getCurrentIndex() == -1)
		{
			// 末尾のアイテムをカレントにする。
			m_tab.setCurrentIndex(m_tab.getTabCount() - 1);
		}

		m_tab.changeCurrent();
	}

	// ウィンドウをフローティング状態にする。
	window->floatWindow();
}

void Pane::removeAllWindows()
{
	// すべてのウィンドウをフローティング状態にする。
	int c = m_tab.getTabCount();
	for (int i = 0; i < c; i++)
	{
		Window* window = m_tab.getWindow(i);

		window->m_pane = 0;
		window->floatWindow();
	}

	// すべてのタブを削除する。
	m_tab.deleteAllTabs();
}

// ペインをリセットする。
void Pane::resetPane()
{
	removeAllWindows();

	for (auto& child : m_children)
	{
		if (!child) continue;

		child->resetPane();
		child = 0;
	}
}

// ペインの分割モードを変更する。
void Pane::setSplitMode(int splitMode)
{
	m_splitMode = splitMode;

	switch (m_splitMode)
	{
	case SplitMode::none:
		{
			resetPane();

			break;
		}
	case SplitMode::vert:
		{
			m_border = getWidth(m_position) / 2;

			if (!m_children[0]) m_children[0].reset(new Pane());
			if (!m_children[1]) m_children[1].reset(new Pane());

			break;
		}
	case SplitMode::horz:
		{
			m_border = getHeight(m_position) / 2;

			if (!m_children[0]) m_children[0].reset(new Pane());
			if (!m_children[1]) m_children[1].reset(new Pane());

			break;
		}
	}

	removeAllWindows();
}

inline int clamp(int x, int minValue, int maxValue)
{
	if (x < minValue) return minValue;
	if (x > maxValue) return maxValue;
	return x;
}

int Pane::absoluteX(int x)
{
	switch (m_origin)
	{
	case Origin::topLeft: return m_position.left + x;
	case Origin::bottomRight: return m_position.right - x - g_borderWidth;
	}

	return 0;
}

int Pane::absoluteY(int y)
{
	switch (m_origin)
	{
	case Origin::topLeft: return m_position.top + y;
	case Origin::bottomRight: return m_position.bottom - y - g_borderWidth;
	}

	return 0;
}

int Pane::relativeX(int x)
{
	switch (m_origin)
	{
	case Origin::topLeft: return x - m_position.left;
	case Origin::bottomRight: return m_position.right - x + g_borderWidth;
	}

	return 0;
}

int Pane::relativeY(int y)
{
	switch (m_origin)
	{
	case Origin::topLeft: return y - m_position.top;
	case Origin::bottomRight: return m_position.bottom - y + g_borderWidth;
	}

	return 0;
}

RECT Pane::getCaptionRect()
{
	return RECT
	{
		m_position.left,
		m_position.top,
		m_position.right,
		m_position.top + g_captionHeight,
	};
}

RECT Pane::getMenuRect()
{
	return RECT
	{
		m_position.left,
		m_position.top,
		m_position.left + g_captionHeight,
		m_position.top + g_captionHeight,
	};
}

void Pane::normalize()
{
//	MY_TRACE(_T("Pane::normalize()\n"));

	switch (m_splitMode)
	{
	case SplitMode::vert: m_border = clamp(m_border, 0, getWidth(m_position) - g_borderWidth); break;
	case SplitMode::horz: m_border = clamp(m_border, 0, getHeight(m_position) - g_borderWidth); break;
	}
}

void Pane::recalcLayout()
{
	recalcLayout(&m_position);
}

void Pane::recalcLayout(LPCRECT rc)
{
//	MY_TRACE(_T("Pane::recalcLayout()\n"));

	if (::IsIconic(g_singleWindow))
		return;

	// このペインの位置を更新する。
	m_position = *rc;

	int c = m_tab.getTabCount();

	// ウィンドウを 2 個以上持っているなら
	if (c >= 2)
	{
		int x = m_position.left + g_captionHeight;
		int y = m_position.top;
		int w = getWidth(m_position) - g_captionHeight;
		int h = g_captionHeight;

		// タブコントロールを表示する。
		true_SetWindowPos(m_tab.m_hwnd, 0,
			x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
	}
	// ウィンドウの数が 1 個以下なら
	else
	{
		// タブコントロールを非表示にする。
		::ShowWindow(m_tab.m_hwnd, SW_HIDE);
	}

	if (c) // ウィンドウを持っている場合は
	{
		for (int i = 0; i < c; i++)
		{
			Window* window = m_tab.getWindow(i);

			window->resizeDockContainer(&m_position); // ウィンドウをリサイズする。
		}

		return; // ウィンドウを持つペインは子ペインを持たないのでここで終了する。
	}

	// ボーダーが飛び出ないように調整する。
	normalize();

	switch (m_splitMode)
	{
	case SplitMode::vert:
		{
			int border = absoluteX(m_border);

			if (m_children[0])
			{
				RECT rc = { m_position.left, m_position.top, border, m_position.bottom };

				m_children[0]->recalcLayout(&rc);
			}

			if (m_children[1])
			{
				RECT rc = { border + g_borderWidth, m_position.top, m_position.right, m_position.bottom };

				m_children[1]->recalcLayout(&rc);
			}

			break;
		}
	case SplitMode::horz:
		{
			int border = absoluteY(m_border);

			if (m_children[0])
			{
				RECT rc = { m_position.left, m_position.top, m_position.right, border };

				m_children[0]->recalcLayout(&rc);
			}

			if (m_children[1])
			{
				RECT rc = { m_position.left, border + g_borderWidth, m_position.right, m_position.bottom };

				m_children[1]->recalcLayout(&rc);
			}

			break;
		}
	}
}

PanePtr Pane::hitTestPane(POINT point)
{
	// point がこのペインの範囲外なら
	if (!::PtInRect(&m_position, point))
		return 0; // ヒットしない。

	// このペインがウィンドウを持っているなら
	if (m_tab.getTabCount())
		return shared_from_this(); // ヒットする。

	switch (m_splitMode)
	{
	case SplitMode::vert:
	case SplitMode::horz:
		{
			// 子ペインの関数を再帰呼び出しする。
			for (auto& child : m_children)
			{
				if (!child) continue;

				PanePtr retValue = child->hitTestPane(point);
				if (retValue) return retValue;
			}

			break;
		}
	}

	return shared_from_this();
}

PanePtr Pane::hitTestBorder(POINT point)
{
	// point がこのペインの範囲外なら
	if (!::PtInRect(&m_position, point))
		return 0; // ヒットしない。

	// このペインがウィンドウを持っているなら
	if (m_tab.getTabCount())
		return 0; // ヒットしない。

	switch (m_splitMode)
	{
	case SplitMode::vert:
		{
			int border = absoluteX(m_border);

			// point がボーダーの範囲内なら
			if (point.x >= border && point.x < border + g_borderWidth)
				return shared_from_this(); // ヒットする。

			break;
		}
	case SplitMode::horz:
		{
			int border = absoluteY(m_border);

			// point がボーダーの範囲内なら
			if (point.y >= border && point.y < border + g_borderWidth)
				return shared_from_this(); // ヒットする。

			break;
		}
	default:
		{
			return 0;
		}
	}

	// 子ペインの関数を再帰呼び出しする。
	for (auto& child : m_children)
	{
		if (!child) continue;

		PanePtr retValue = child->hitTestBorder(point);
		if (retValue) return retValue;
	}

	return 0;
}

int Pane::getDragOffset(POINT point)
{
	switch (m_splitMode)
	{
	case SplitMode::vert: return m_border - relativeX(point.x);
	case SplitMode::horz: return m_border - relativeY(point.y);
	}

	return 0;
}

void Pane::dragBorder(POINT point)
{
	switch (m_splitMode)
	{
	case SplitMode::vert: m_border = relativeX(point.x) + m_dragOffset; break;
	case SplitMode::horz: m_border = relativeY(point.y) + m_dragOffset; break;
	}
}

BOOL Pane::getBorderRect(LPRECT rc)
{
	// ウィンドウを持っている場合は
	if (m_tab.getTabCount())
		return FALSE; // ボーダーを持たない。

	switch (m_splitMode)
	{
	case SplitMode::vert:
		{
			int border = absoluteX(m_border);

			rc->left = border;
			rc->top = m_position.top;
			rc->right = border + g_borderWidth;
			rc->bottom = m_position.bottom;

			return TRUE;
		}
	case SplitMode::horz:
		{
			int border = absoluteY(m_border);

			rc->left = m_position.left;
			rc->top = border;
			rc->right = m_position.right;
			rc->bottom = border + g_borderWidth;

			return TRUE;
		}
	}

	return FALSE;
}

void Pane::drawBorder(HDC dc, HBRUSH brush)
{
	// ウィンドウを持っている場合は
	if (m_tab.getTabCount())
		return; // ボーダーを持たない。

	switch (m_splitMode)
	{
	case SplitMode::vert:
	case SplitMode::horz:
		{
			RECT rc;
			if (getBorderRect(&rc))
			{
				// テーマを使用するなら
				if (g_useTheme)
				{
					int partId = WP_BORDER;
					int stateId = CS_INACTIVE;

					// テーマ API を使用してボーダーを描画する。
					::DrawThemeBackground(g_theme, dc, partId, stateId, &rc, 0);
				}
				// テーマを使用しないなら
				else
				{
					// ブラシで塗り潰す。
					::FillRect(dc, &rc, brush);
				}
			}

			for (auto& child : m_children)
			{
				if (!child) continue;

				child->drawBorder(dc, brush);
			}

			break;
		}
	}
}

void Pane::drawCaption(HDC dc)
{
	Window* window = getActiveWindow();

	if (window)
	{
		RECT rc = getCaptionRect();

		if (rc.bottom > m_position.bottom)
			return; // キャプションがはみ出てしまう場合は何もしない。

		RECT rcMenu = getMenuRect();
		BOOL hasMenu = !!::GetMenu(window->m_hwnd);

		// ウィンドウテキストを取得する。
		WCHAR text[MAX_PATH] = {};
		::GetWindowTextW(window->m_hwnd, text, MAX_PATH);

		// テーマを使用するなら
		if (g_useTheme)
		{
			// ウィンドウの状態から stateId を取得する。
			int stateId = CS_ACTIVE;
			if (::GetFocus() != window->m_hwnd) stateId = CS_INACTIVE;
			if (!::IsWindowEnabled(window->m_hwnd)) stateId = CS_DISABLED;

			// テーマ API を使用してタイトルを描画する。
			::DrawThemeBackground(g_theme, dc, WP_CAPTION, stateId, &rc, 0);

			{
				RECT rcText = rc;

				if (hasMenu)
				{
					rcText.left = rcMenu.right;

					::DrawThemeText(g_theme, dc, WP_CAPTION, stateId,
						L"M", -1, DT_CENTER | DT_VCENTER | DT_SINGLELINE, 0, &rcMenu);
				}

				if (rcText.left < rcText.right)
					::DrawThemeText(g_theme, dc, WP_CAPTION, stateId,
						text, ::lstrlenW(text), DT_CENTER | DT_VCENTER | DT_SINGLELINE, 0, &rcText);
			}
		}
		// テーマを使用しないなら
		else
		{
			// 直接 GDI を使用して描画する。

			COLORREF captionColor = g_activeCaptionColor;
			COLORREF captionTextColor = g_activeCaptionTextColor;

			if (::GetFocus() != window->m_hwnd)
			{
				captionColor = g_inactiveCaptionColor;
				captionTextColor = g_inactiveCaptionTextColor;
			}

			HBRUSH brush = ::CreateSolidBrush(captionColor);
			::FillRect(dc, &rc, brush);
			::DeleteObject(brush);

			int bkMode = ::SetBkMode(dc, TRANSPARENT);
			COLORREF textColor = ::SetTextColor(dc, captionTextColor);
			{
				RECT rcText = rc;

				if (hasMenu)
				{
					rcText.left = rcMenu.right;

					::DrawTextW(dc, L"M", -1, &rcMenu, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
				}

				if (rcText.left < rcText.right)
					::DrawTextW(dc, text, ::lstrlenW(text), &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}
			::SetTextColor(dc, textColor);
			::SetBkMode(dc, bkMode);
		}

		return;
	}

	switch (m_splitMode)
	{
	case SplitMode::vert:
	case SplitMode::horz:
		{
			for (auto& child : m_children)
			{
				if (!child) continue;

				child->drawCaption(dc);
			}

			break;
		}
	}
}

//---------------------------------------------------------------------
