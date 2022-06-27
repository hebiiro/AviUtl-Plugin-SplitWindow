﻿#include "pch.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

// ペインをリセットする。
void Pane::resetPane()
{
	if (m_window)
	{
		m_window->m_pane = 0;
		m_window->floatWindow();
		m_window = 0;
	}

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

	setWindow(0);
}

// ペインにウィンドウをドッキングする。
void Pane::setWindow(WindowPtr newWindow)
{
	WindowPtr oldWindow = m_window;

	// 古いウィンドウと新しいウィンドウが同じなので何もしない。
	if (oldWindow == newWindow) return;

	// 古いウィンドウが存在するなら
	if (oldWindow)
	{
		// 古いウィンドウと新しいウィンドウがどちらもドッキング状態なら
		if (oldWindow->m_pane && newWindow && newWindow->m_pane)
		{
			// ドッキングウィンドウをスワップするための処理。

			// 古いウィンドウを新しいウィンドウの古いペインにドッキングさせる。
			oldWindow->m_pane = newWindow->m_pane;
			oldWindow->m_pane->m_window = oldWindow;
			oldWindow->dockWindow(&oldWindow->m_pane->m_position);

			// 新しいウィンドウをドッキングする。
			m_window = newWindow;
			m_window->m_pane = this;
			m_window->dockWindow(&m_position);

			return;
		}
		else
		{
			// 古いウィンドウをフローティングさせる。
			oldWindow->m_pane = 0;
			oldWindow->floatWindow();
		}
	}

	// 新しいウィンドウをセットする。
	m_window = newWindow;

	// 新しいウィンドウが存在するなら
	if (m_window)
	{
		// 新しいウィンドウがドッキング状態なら
		if (m_window->m_pane && m_window->m_pane != this)
		{
			// 新しいウィンドウと古いペインのドッキングを解除させる。
			m_window->m_pane->m_window = 0;
		}

		// 新しいウィンドウをドッキングする。
		m_window->m_pane = this;
		m_window->dockWindow(&m_position);
	}
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

	// ウィンドウを持っている場合は
	if (m_window)
	{
		m_window->resizeDockContainer(&m_position); // ウィンドウをリサイズする。

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

	// このペインがウィンドウを持つなら
	if (m_window)
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

	// このペインがウィンドウを持つなら
	if (m_window)
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
	if (m_window)
		return FALSE;

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
	if (m_window)
		return;

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
	if (m_window)
	{
		RECT rc = getCaptionRect();

		if (rc.bottom > m_position.bottom)
			return; // キャプションがはみ出てしまう場合は何もしない。

		RECT rcMenu = getMenuRect();
		BOOL hasMenu = !!::GetMenu(m_window->m_hwnd);

		// ウィンドウテキストを取得する。
		WCHAR text[MAX_PATH] = {};
		::GetWindowTextW(m_window->m_hwnd, text, MAX_PATH);

		// テーマを使用するなら
		if (g_useTheme)
		{
			// ウィンドウの状態から stateId を取得する。
			int stateId = CS_ACTIVE;
			if (::GetFocus() != m_window->m_hwnd) stateId = CS_INACTIVE;
			if (!::IsWindowEnabled(m_window->m_hwnd)) stateId = CS_DISABLED;

			// テーマ API を使用してタイトルを描画する。
			::DrawThemeBackground(g_theme, dc, WP_CAPTION, stateId, &rc, 0);
			::DrawThemeText(g_theme, dc, WP_CAPTION, stateId,
				text, ::lstrlenW(text), DT_CENTER | DT_VCENTER | DT_SINGLELINE, 0, &rc);
			if (hasMenu)
			{
				::DrawThemeText(g_theme, dc, WP_CAPTION, stateId,
					L"M", -1, DT_CENTER | DT_VCENTER | DT_SINGLELINE, 0, &rc);
			}
		}
		// テーマを使用しないなら
		else
		{
			// 直接 GDI を使用して描画する。

			COLORREF captionColor = g_activeCaptionColor;
			COLORREF captionTextColor = g_activeCaptionTextColor;

			if (::GetFocus() != m_window->m_hwnd)
			{
				captionColor = g_inactiveCaptionColor;
				captionTextColor = g_inactiveCaptionTextColor;
			}

			HBRUSH brush = ::CreateSolidBrush(captionColor);
			::FillRect(dc, &rc, brush);
			::DeleteObject(brush);

			int bkMode = ::SetBkMode(dc, TRANSPARENT);
			COLORREF textColor = ::SetTextColor(dc, captionTextColor);
			::DrawTextW(dc, text, ::lstrlenW(text), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			if (hasMenu) ::DrawTextW(dc, L"M", -1, &rcMenu, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
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
