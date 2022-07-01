#pragma once

//---------------------------------------------------------------------

struct SplitMode
{
	static const int none = 0;
	static const int vert = 1;
	static const int horz = 2;
};

static const Label g_splitModeLabel[] = 
{
	{ L"none", SplitMode::none },
	{ L"vert", SplitMode::vert },
	{ L"horz", SplitMode::horz },
};

struct Origin
{
	static const int topLeft = 0;
	static const int bottomRight = 1;
};

static const Label g_originLabel[] = 
{
	{ L"topLeft", Origin::topLeft },
	{ L"bottomRight", Origin::bottomRight },
};

class Pane; typedef std::shared_ptr<Pane> PanePtr;
class Window; typedef std::shared_ptr<Window> WindowPtr;
class Container; typedef std::shared_ptr<Container> ContainerPtr;

typedef std::map<_bstr_t, WindowPtr> WindowMap;

class AviUtlWindow; typedef std::shared_ptr<AviUtlWindow> AviUtlWindowPtr;
class ExEditWindow; typedef std::shared_ptr<ExEditWindow> ExEditWindowPtr;
class SettingDialog; typedef std::shared_ptr<SettingDialog> SettingDialogPtr;

class TabControl
{
public:

	HWND m_hwnd;

	TabControl(Pane* pane);
	~TabControl();

	static Pane* getPane(HWND hwnd);
	static void setPane(HWND hwnd, Pane* pane);

	int getTabCount();
	Window* getWindow(int index);
	int getCurrentIndex();
	int setCurrentIndex(int index);
	int hitTest(POINT point);
	int addTab(Window* window, LPCTSTR text, int index);
	void deleteTab(int index);
	void deleteAllTabs();
	int findTab(Window* window);
	int moveTab(int from, int to);
	void changeCurrent();
	void changeText(Window* window, LPCTSTR text);
};

class Pane : public std::enable_shared_from_this<Pane>
{
public:

	RECT m_position = {};
	int m_splitMode = SplitMode::none;
	int m_origin = Origin::bottomRight;
	int m_border = 0;
	int m_dragOffset = 0; // ドラッグ処理に使う。

	TabControl m_tab;
	PanePtr m_children[2];

	Pane();
	~Pane();

	Window* getActiveWindow();
	int addWindow(Window* window, int index = -1);
	void removeWindow(Window* window);
	void removeAllWindows();

	void resetPane();
	void setSplitMode(int splitMode);
	int absoluteX(int x);
	int absoluteY(int x);
	int relativeX(int x);
	int relativeY(int x);
	RECT getDockRect();
	RECT getCaptionRect();
	RECT getMenuRect();
	void normalize();
	void recalcLayout();
	void recalcLayout(LPCRECT rc);
	PanePtr hitTestPane(POINT point);
	PanePtr hitTestBorder(POINT point);
	int getDragOffset(POINT point);
	void dragBorder(POINT point);
	BOOL getBorderRect(LPRECT rc);
	void drawBorder(HDC dc, HBRUSH brush);
	void drawCaption(HDC dc);
};

//---------------------------------------------------------------------
