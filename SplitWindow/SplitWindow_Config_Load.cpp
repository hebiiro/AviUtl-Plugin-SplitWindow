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
			getPrivateProfileInt(element, L"tabHeight", g_tabHeight);
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
			getPrivateProfileBool(element, L"forceScroll", g_forceScroll);
		}

		// <hub> を読み込む。
		loadHub(element);

		// <colony> を読み込む。
		loadColony(element);

		// <floatShuttle> を読み込む。
		loadFloatShuttle(element);

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

// <hub> を読み込む。
HRESULT loadHub(const MSXML2::IXMLDOMElementPtr& element)
{
	MY_TRACE(_T("loadHub()\n"));

	// <hub> を読み込む。
	MSXML2::IXMLDOMNodeListPtr nodeList = element->selectNodes(L"hub");
	int c = nodeList->length;
	for (int i = 0; i < c; i++)
	{
		MSXML2::IXMLDOMElementPtr hubElement = nodeList->item[i];

		HWND hwndHub = g_hub;
		PanePtr root = getRootPane(hwndHub);

		// 一旦すべてのペインをリセットする。
		root->resetPane();

		// ウィンドウ位置を読み込む。
		getPrivateProfileWindow(hubElement, L"placement", hwndHub);

		// <pane> を読み込む。
		MSXML2::IXMLDOMNodeListPtr nodeList = hubElement->selectNodes(L"pane");
		int c = min(1, nodeList->length);
		for (int i = 0; i < c; i++)
		{
			MSXML2::IXMLDOMElementPtr paneElement = nodeList->item[i];

			loadPane(paneElement, root);
		}
	}

	return S_OK;
}

// <colony> を読み込む。
HRESULT loadColony(const MSXML2::IXMLDOMElementPtr& element)
{
	MY_TRACE(_T("loadColony()\n"));

	// 一旦すべてのコロニーを削除する。
	g_colonySet.clear();

	// <colony> を読み込む。
	MSXML2::IXMLDOMNodeListPtr nodeList = element->selectNodes(L"colony");
	int c = nodeList->length;
	for (int i = 0; i < c; i++)
	{
		MSXML2::IXMLDOMElementPtr colonyElement = nodeList->item[i];

		HWND hwndColony = createColony();
		PanePtr root = getRootPane(hwndColony);

		// ウィンドウテキストを読み込む。
		_bstr_t name = L"";
		getPrivateProfileName(colonyElement, L"name", name);
		::SetWindowText(hwndColony, name);

		// ウィンドウ位置を読み込む。
		getPrivateProfileWindow(colonyElement, L"placement", hwndColony);

		// コロニーが非表示なら
		if (!::IsWindowVisible(hwndColony))
		{
			// 強制的に表示状態にする。
			::ShowWindow(hwndColony, SW_SHOWNA);
		}

		// <pane> を読み込む。
		MSXML2::IXMLDOMNodeListPtr nodeList = colonyElement->selectNodes(L"pane");
		int c = min(1, nodeList->length);
		for (int i = 0; i < c; i++)
		{
			MSXML2::IXMLDOMElementPtr paneElement = nodeList->item[i];

			loadPane(paneElement, root);
		}
	}

	return S_OK;
}

// <pane> を読み込む。
HRESULT loadPane(const MSXML2::IXMLDOMElementPtr& paneElement, PanePtr pane)
{
	MY_TRACE(_T("loadPane()\n"));

	// <pane> のアトリビュートを読み込む。

	int current = -1;

	getPrivateProfileLabel(paneElement, L"splitMode", pane->m_splitMode, g_splitModeLabel);
	getPrivateProfileLabel(paneElement, L"origin", pane->m_origin, g_originLabel);
	getPrivateProfileInt(paneElement, L"isBorderLocked", pane->m_isBorderLocked);
	getPrivateProfileInt(paneElement, L"border", pane->m_border);
	getPrivateProfileInt(paneElement, L"current", current);

	MY_TRACE_INT(pane->m_splitMode);
	MY_TRACE_INT(pane->m_origin);
	MY_TRACE_INT(pane->m_border);
	MY_TRACE_INT(current);

	{
		// <dockShuttle> を読み込む。
		MSXML2::IXMLDOMNodeListPtr nodeList = paneElement->selectNodes(L"dockShuttle");
		int c = nodeList->length;
		for (int i = 0; i < c; i++)
		{
			MSXML2::IXMLDOMElementPtr dockShuttleElement = nodeList->item[i];

			_bstr_t name = L"";
			getPrivateProfileName(dockShuttleElement, L"name", name);
			MY_TRACE_WSTR((LPCWSTR)name);

			auto it = g_shuttleMap.find(name);
			if (it != g_shuttleMap.end())
				pane->addShuttle(it->second.get());
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

		pane->m_children[i].reset(new Pane(pane->m_owner));
		loadPane(paneElement, pane->m_children[i]);
	}

	return S_OK;
}

// <floatShuttle> を読み込む。
HRESULT loadFloatShuttle(const MSXML2::IXMLDOMElementPtr& element)
{
	MY_TRACE(_T("loadFloatShuttle()\n"));

	// <floatShuttle> を読み込む。
	MSXML2::IXMLDOMNodeListPtr nodeList = element->selectNodes(L"floatShuttle");
	int c = nodeList->length;
	for (int i = 0; i < c; i++)
	{
		MSXML2::IXMLDOMElementPtr floatShuttleElement = nodeList->item[i];

		_bstr_t name = L"";
		getPrivateProfileName(floatShuttleElement, L"name", name);

		auto it = g_shuttleMap.find(name);
		if (it != g_shuttleMap.end())
		{
			ShuttlePtr shuttle = it->second;

			getPrivateProfileWindow(floatShuttleElement, L"placement", shuttle->m_floatContainer->m_hwnd);

			// フローティングコンテナが表示状態なら
			if (::IsWindowVisible(shuttle->m_floatContainer->m_hwnd))
			{
				// ドッキング状態なら
				if (shuttle->m_pane)
				{
					// ドッキングを解除する。
					shuttle->m_pane->removeShuttle(shuttle.get());
				}

				// ターゲットウィンドウを表示状態にする。
				shuttle->showTargetWindow();
			}
		}
	}

	return S_OK;
}

//---------------------------------------------------------------------
