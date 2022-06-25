#include "pch.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

HRESULT saveConfig()
{
	MY_TRACE(_T("saveConfig()\n"));

	WCHAR fileName[MAX_PATH] = {};
	::GetModuleFileNameW(g_instance, fileName, MAX_PATH);
	::PathRenameExtensionW(fileName, L".xml");

	return saveConfig(fileName, FALSE);
}

HRESULT saveConfig(LPCWSTR fileName, BOOL _export)
{
	MY_TRACE(_T("saveConfig(%ws, %d)\n"), fileName, _export);

	try
	{
		// ドキュメントを作成する。
		MSXML2::IXMLDOMDocumentPtr document(__uuidof(MSXML2::DOMDocument));

		// ドキュメントエレメントを作成する。
		MSXML2::IXMLDOMElementPtr element = appendElement(document, document, L"config");

		if (!_export) // エクスポートのときはこれらの変数は保存しない。
		{
			setPrivateProfileInt(element, L"borderWidth", g_borderWidth);
			setPrivateProfileInt(element, L"captionHeight", g_captionHeight);
			setPrivateProfileInt(element, L"borderSnapRange", g_borderSnapRange);
			setPrivateProfileColor(element, L"fillColor", g_fillColor);
			setPrivateProfileColor(element, L"borderColor", g_borderColor);
			setPrivateProfileColor(element, L"hotBorderColor", g_hotBorderColor);
			setPrivateProfileColor(element, L"activeCaptionColor", g_activeCaptionColor);
			setPrivateProfileColor(element, L"activeCaptionTextColor", g_activeCaptionTextColor);
			setPrivateProfileColor(element, L"inactiveCaptionColor", g_inactiveCaptionColor);
			setPrivateProfileColor(element, L"inactiveCaptionTextColor", g_inactiveCaptionTextColor);
			setPrivateProfileBool(element, L"useTheme", g_useTheme);
		}

		// <singleWindow> にウィンドウ位置を保存する。
		setPrivateProfileWindow(element, L"singleWindow", g_singleWindow);

		// <pane> を作成する。
		savePane(element, g_root);

		// <floatWindow> を作成する。
		saveFloatWindow(element);

		return saveXMLDocument(document, fileName, L"UTF-16");
	}
	catch (_com_error& e)
	{
		MY_TRACE(_T("%s\n"), e.ErrorMessage());
		return e.Error();
	}
}

// <pane> を作成する。
HRESULT savePane(const MSXML2::IXMLDOMElementPtr& element, PanePtr pane)
{
	MY_TRACE(_T("savePane()\n"));

	// <pane> を作成する。
	MSXML2::IXMLDOMElementPtr paneElement = appendElement(element, L"pane");

	setPrivateProfileLabel(paneElement, L"splitMode", pane->m_splitMode, g_splitModeLabel);
	setPrivateProfileLabel(paneElement, L"origin", pane->m_origin, g_originLabel);
	setPrivateProfileInt(paneElement, L"border", pane->m_border);

	if (pane->m_window)
	{
		for (auto& x : g_windowMap)
		{
			if (pane->m_window == x.second)
			{
				std::wstring name(x.first);
				std::wstring t = L"* ";  // 検索文字列
				auto pos = name.find(t);  // 検索文字列が見つかった位置
				auto len = t.length(); // 検索文字列の長さ
				if (pos != std::string::npos)
					name.replace(pos, len, L"");

				setPrivateProfileString(paneElement, L"window", name.c_str());

				break;
			}
		}
	}

	for (auto& child : pane->m_children)
	{
		if (child)
			savePane(paneElement, child);
	}

	return S_OK;
}

// <floatWindow> を作成する。
HRESULT saveFloatWindow(const MSXML2::IXMLDOMElementPtr& element)
{
	MY_TRACE(_T("saveFloatWindow()\n"));

	for (auto x : g_windowMap)
	{
		// <floatWindow> を作成する。
		MSXML2::IXMLDOMElementPtr floatWindowElement = appendElement(element, L"floatWindow");

		std::wstring name(x.first);
		std::wstring t = L"* ";  // 検索文字列
		auto pos = name.find(t);  // 検索文字列が見つかった位置
		auto len = t.length(); // 検索文字列の長さ
		if (pos != std::string::npos)
			name.replace(pos, len, L"");

		setPrivateProfileString(floatWindowElement, L"name", name.c_str());
		setPrivateProfileWindow(floatWindowElement, L"placement", x.second->m_floatContainer->m_hwnd);
	}

	return S_OK;
}

//---------------------------------------------------------------------
