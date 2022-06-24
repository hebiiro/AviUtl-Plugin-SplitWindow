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

class Pane : public std::enable_shared_from_this<Pane>
{
public:

	RECT m_position = {};
	int m_splitMode = SplitMode::none;
	int m_origin = Origin::bottomRight;
	int m_border = 0;
	int m_dragOffset = 0; // ドラッグ処理に使う。

	WindowPtr m_window;
	PanePtr m_children[2];

	void resetPane();
	void setSplitMode(int splitMode);
	void setWindow(WindowPtr window);
	int mirrorX(int x);
	int mirrorY(int x);
	RECT getCaptionRect();
	void normalize();
	void recalcLayout();
	void recalcLayout(LPCRECT rc);
	PanePtr hitTest(POINT point);
	PanePtr hitTestBorder(POINT point);
	int getDragOffset(POINT point);
	void dragBorder(POINT point);
	BOOL getBorderRect(LPRECT rc);
	void drawBorder(HDC dc, HBRUSH brush);
	void drawCaption(HDC dc);
};

//---------------------------------------------------------------------
