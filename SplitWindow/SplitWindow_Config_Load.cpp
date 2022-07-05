#include "pch.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

HRESULT WINAPI getPrivateProfileName(const MSXML2::IXMLDOMElementPtr& element, LPCWSTR name, _bstr_t& outValue)
{
	HRESULT hr = getPrivateProfileString(element->getAttribute(name), outValue);
	if (hr == S_OK)
	{
		if (::lstrcmpW(outValue, L"AviUtl") == 0 ||
			::lstrcmpW(outValue, L"拡張編集") == 0 ||
			::lstrcmpW(outValue, L"設定ダイアログ") == 0)
		{
			outValue = L"* " + outValue;
		}
	}
	return hr;
}

HRESULT loadConfig()
{
	MY_TRACE(_T("loadConfig()\n"));

	WCHAR fileName[MAX_PATH] = {};
	::GetModuleFileNameW(g_instance, fileName, MAX_PATH);
	::PathRenameExtensionW(fileName, L".xml");

	return loadConfig(fileName, FALSE);
}

HRESULT loadConfig(LPCWSTR fileName, BOOL _import)
{
	MY_TRACE(_T("loadConfig(%ws, %d)\n"), fileName, _import);

	try
	{
		// MSXML を使用する。
		MSXML2::IXMLDOMDocumentPtr document(__uuidof(MSXML2::DOMDocument));

		// 設定ファイルを開く。
		if (document->load(fileName) == VARIANT_FALSE)
		{
			MY_TRACE(_T("%s を開けませんでした\n"), fileName);

			return S_FALSE;
		}

		MSXML2::IXMLDOMElementPtr element = document->documentElement;

		if (!_import) // インポートのときはこれらの変数は取得しない。
		{
			getPrivateProfileInt(element, L"borderWidth", g_borderWidth);
			getPrivateProfileInt(element, L"captionHeight", g_captionHeight);
			getPrivateProfileInt(element, L"menuBreak", g_menuBreak);
			getPrivateProfileLabel(element, L"tabMode", g_tabMode, g_tabModeLabel);
			getPrivateProfileColor(element, L"fillColor", g_fillColor);
			getPrivateProfileColor(element, L"borderColor", g_borderColor);
			getPrivateProfileColor(element, L"hotBorderColor", g_hotBorderColor);
			getPrivateProfileColor(element, L"activeCaptionColor", g_activeCaptionColor);
			getPrivateProfileColor(element, L"activeCaptionTextColor", g_activeCaptionTextColor);
			getPrivateProfileColor(element, L"inactiveCaptionColor", g_inactiveCaptionColor);
			getPrivateProfileColor(element, L"inactiveCaptionTextColor", g_inactiveCaptionTextColor);
			getPrivateProfileBool(element, L"useTheme", g_useTheme);
		}

		// 一旦すべてのペインをリセットする。
		g_root->resetPane();

		// ウィンドウ位置を取得する。
		getPrivateProfileWindow(element, L"singleWindow", g_singleWindow);

		// <pane> を読み込む。
		MSXML2::IXMLDOMNodeListPtr nodeList = element->selectNodes(L"pane");
		int c = min(1, nodeList->length);
		for (int i = 0; i < c; i++)
		{
			MSXML2::IXMLDOMElementPtr paneElement = nodeList->item[i];

			loadPane(paneElement, g_root);
		}

		// <floatWindow> を読み込む。
		loadFloatWindow(element);

		MY_TRACE(_T("設定ファイルの読み込みに成功しました\n"));

		return S_OK;
	}
	catch (_com_error& e)
	{
		MY_TRACE(_T("設定ファイルの読み込みに失敗しました\n"));
		MY_TRACE(_T("%s\n"), e.ErrorMessage());
		return e.Error();
	}
}

// <pane> を読み込む。
HRESULT loadPane(const MSXML2::IXMLDOMElementPtr& paneElement, PanePtr pane)
{
	MY_TRACE(_T("loadPane()\n"));

	// <pane> のアトリビュートを読み込む。

	int current = -1;

	getPrivateProfileLabel(paneElement, L"splitMode", pane->m_splitMode, g_splitModeLabel);
	getPrivateProfileLabel(paneElement, L"origin", pane->m_origin, g_originLabel);
	getPrivateProfileInt(paneElement, L"border", pane->m_border);
	getPrivateProfileInt(paneElement, L"current", current);

	MY_TRACE_INT(pane->m_splitMode);
	MY_TRACE_INT(pane->m_origin);
	MY_TRACE_INT(pane->m_border);
	MY_TRACE_INT(current);

	{
		// <dockWindow> を読み込む。
		MSXML2::IXMLDOMNodeListPtr nodeList = paneElement->selectNodes(L"dockWindow");
		int c = nodeList->length;
		for (int i = 0; i < c; i++)
		{
			MSXML2::IXMLDOMElementPtr dockWindowElement = nodeList->item[i];

			_bstr_t name = L"";
			getPrivateProfileName(dockWindowElement, L"name", name);
			MY_TRACE_WSTR((LPCWSTR)name);

			auto it = g_windowMap.find(name);
			if (it != g_windowMap.end())
				pane->addWindow(it->second.get());
		}
	}

	pane->m_tab.setCurrentIndex(current);
	pane->m_tab.changeCurrent();

	// <pane> を読み込む。
	MSXML2::IXMLDOMNodeListPtr nodeList = paneElement->selectNodes(L"pane");
	int c = min(2, nodeList->length);
	for (int i = 0; i < c; i++)
	{
		MSXML2::IXMLDOMElementPtr paneElement = nodeList->item[i];

		pane->m_children[i].reset(new Pane());
		loadPane(paneElement, pane->m_children[i]);
	}

	return S_OK;
}

// <floatWindow> を読み込む。
HRESULT loadFloatWindow(const MSXML2::IXMLDOMElementPtr& element)
{
	MY_TRACE(_T("loadFloatWindow()\n"));

	// <floatWindow> を読み込む。
	MSXML2::IXMLDOMNodeListPtr nodeList = element->selectNodes(L"floatWindow");
	int c = nodeList->length;
	for (int i = 0; i < c; i++)
	{
		MSXML2::IXMLDOMElementPtr floatWindowElement = nodeList->item[i];

		_bstr_t name = L"";
		getPrivateProfileName(floatWindowElement, L"name", name);

		auto it = g_windowMap.find(name);
		if (it != g_windowMap.end())
		{
			WindowPtr window = it->second;

			getPrivateProfileWindow(floatWindowElement, L"placement", window->m_floatContainer->m_hwnd);

			// フローティングコンテナが表示状態なら
			if (::IsWindowVisible(window->m_floatContainer->m_hwnd))
			{
				// ターゲットウィンドウを表示状態にする。
				window->showTargetWindow();
			}
		}
	}

	return S_OK;
}

//---------------------------------------------------------------------
