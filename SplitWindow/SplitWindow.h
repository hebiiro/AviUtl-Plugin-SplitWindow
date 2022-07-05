#pragma once

//---------------------------------------------------------------------

struct Label { LPCWSTR label; int value; };

//---------------------------------------------------------------------

#include "Pane.h"
#include "Shuttle.h"
#include "Container.h"

//---------------------------------------------------------------------

struct WindowMessage
{
	static const UINT WM_POST_INIT = WM_APP + 1;
};

struct CommandID
{
	static const UINT SHOW_CONFIG_DIALOG = 1000;
	static const UINT IMPORT_LAYOUT = 1001;
	static const UINT EXPORT_LAYOUT = 1002;
	static const UINT CREATE_COLONY = 1003;

	static const UINT SPLIT_MODE_NONE = 1000;
	static const UINT SPLIT_MODE_VERT = 1001;
	static const UINT SPLIT_MODE_HORZ = 1002;
	static const UINT ORIGIN_TOP_LEFT = 1010;
	static const UINT ORIGIN_BOTTOM_RIGHT = 1011;
	static const UINT MOVE_TO_LEFT = 1012;
	static const UINT MOVE_TO_RIGHT = 1013;
	static const UINT WINDOW = 2000;
};

struct TabMode
{
	static const int title = 0;
	static const int top = 1;
	static const int bottom = 2;
};

const Label g_tabModeLabel[] =
{
	{ L"title", TabMode::title },
	{ L"top", TabMode::top },
	{ L"bottom", TabMode::bottom },
};

//---------------------------------------------------------------------

typedef std::set<HWND> HWNDSet;

//---------------------------------------------------------------------

extern AviUtlInternal g_auin;
extern HINSTANCE g_instance;
extern HWND g_hub;
extern HTHEME g_theme;

extern AviUtlWindowPtr g_aviutlWindow;
extern ExEditWindowPtr g_exeditWindow;
extern SettingDialogPtr g_settingDialog;

extern HWNDSet g_colonySet;
extern ShuttleMap g_shuttleMap;
extern PanePtr g_hotBorderPane;

extern int g_borderWidth;
extern int g_captionHeight;
extern int g_menuBreak;
extern int g_tabMode;
extern COLORREF g_fillColor;
extern COLORREF g_borderColor;
extern COLORREF g_hotBorderColor;
extern COLORREF g_activeCaptionColor;
extern COLORREF g_activeCaptionTextColor;
extern COLORREF g_inactiveCaptionColor;
extern COLORREF g_inactiveCaptionTextColor;
extern BOOL g_useTheme;

//---------------------------------------------------------------------

PanePtr getRootPane(HWND hwndColony);
void calcLayout(HWND hwndColony);
void calcAllLayout();
BOOL showTargetMenu(HWND hwndColony, POINT point);
void showPaneMenu(HWND hwndColony);
void fillBackground(HDC dc, LPCRECT rc);

HWND createColony();
LRESULT CALLBACK colonyProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

HWND createHub();
LRESULT CALLBACK hubProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

HRESULT loadConfig();
HRESULT loadConfig(LPCWSTR fileName, BOOL _import);
HRESULT loadHub(const MSXML2::IXMLDOMElementPtr& element);
HRESULT loadColony(const MSXML2::IXMLDOMElementPtr& element);
HRESULT loadPane(const MSXML2::IXMLDOMElementPtr& element, PanePtr pane);
HRESULT loadFloatShuttle(const MSXML2::IXMLDOMElementPtr& element);

HRESULT saveConfig();
HRESULT saveConfig(LPCWSTR fileName, BOOL _export);
HRESULT saveHub(const MSXML2::IXMLDOMElementPtr& element);
HRESULT saveColony(const MSXML2::IXMLDOMElementPtr& element);
HRESULT savePane(const MSXML2::IXMLDOMElementPtr& element, PanePtr pane);
HRESULT saveFloatShuttle(const MSXML2::IXMLDOMElementPtr& element);

//---------------------------------------------------------------------

DECLARE_HOOK_PROC(LRESULT, WINAPI, ComboBoxProc, (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam));
DECLARE_HOOK_PROC(LRESULT, WINAPI, TrackBarProc, (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam));
DECLARE_HOOK_PROC(HWND, WINAPI, CreateWindowExA, (DWORD exStyle, LPCSTR className, LPCSTR windowName, DWORD style, int x, int y, int w, int h, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param));
DECLARE_HOOK_PROC(BOOL, WINAPI, MoveWindow, (HWND hwnd, int x, int y, int w, int h, BOOL repaint));
DECLARE_HOOK_PROC(BOOL, WINAPI, SetWindowPos, (HWND hwnd, HWND insertAfter, int x, int y, int w, int h, UINT flags));
DECLARE_HOOK_PROC(HMENU, WINAPI, GetMenu, (HWND hwnd));
DECLARE_HOOK_PROC(BOOL, WINAPI, SetMenu, (HWND hwnd, HMENU menu));
DECLARE_HOOK_PROC(BOOL, WINAPI, DrawMenuBar, (HWND hwnd));
DECLARE_HOOK_PROC(HWND, WINAPI, FindWindowA, (LPCSTR className, LPCSTR windowName));
DECLARE_HOOK_PROC(HWND, WINAPI, FindWindowW, (LPCWSTR className, LPCWSTR windowName));
DECLARE_HOOK_PROC(HWND, WINAPI, FindWindowExA, (HWND parent, HWND childAfter, LPCSTR className, LPCSTR windowName));
DECLARE_HOOK_PROC(HWND, WINAPI, GetWindow, (HWND hwnd, UINT cmd));
DECLARE_HOOK_PROC(BOOL, WINAPI, EnumThreadWindows, (DWORD threadId, WNDENUMPROC enumProc, LPARAM lParam));
DECLARE_HOOK_PROC(BOOL, WINAPI, EnumWindows, (WNDENUMPROC enumProc, LPARAM lParam));
DECLARE_HOOK_PROC(LONG, WINAPI, SetWindowLongA, (HWND hwnd, int index, LONG newLong));
DECLARE_HOOK_PROC(INT_PTR, CALLBACK, ScriptParamDlgProc, (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam));

COLORREF WINAPI Dropper_GetPixel(HDC dc, int x, int y);
HWND WINAPI KeyboardHook_GetActiveWindow();

// 強制的に WM_SIZE を送信する ::SetWindowPos()。
inline void forceSetWindowPos(HWND hwnd, HWND insertAfter, int x, int y, int w, int h, UINT flags)
{
	RECT rcOld; ::GetWindowRect(hwnd, &rcOld);
	true_SetWindowPos(hwnd, insertAfter, x, y, w, h, flags);

	if (!(flags & SWP_NOSIZE))
	{
		RECT rcNew; ::GetWindowRect(hwnd, &rcNew);

		if (getWidth(rcOld) == getWidth(rcNew) && getHeight(rcOld) == getHeight(rcNew))
		{
			// ウィンドウサイズは変更されなかったが、強制的に WM_SIZE を送信する。
			::SendMessage(hwnd, WM_SIZE, 0, 0);
		}
	}
}

//---------------------------------------------------------------------
