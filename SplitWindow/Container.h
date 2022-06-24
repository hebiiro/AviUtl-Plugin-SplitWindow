#pragma once

//---------------------------------------------------------------------

class Container
{
public:

	HWND m_hwnd = 0;
	Window* m_window = 0;

	Container(Window* window, DWORD style);
	virtual ~Container();

	virtual void onResizeDockContainer(LPCRECT rc);
	virtual void onResizeFloatContainer();
	virtual BOOL onSetContainerPos(WINDOWPOS* wp);
	virtual LRESULT onWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual LRESULT onTargetWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	static BOOL registerWndClass();
	static LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	static Container* getContainer(HWND hwndContainer);
	static void setContainer(HWND hwndContainer, Container* container);
};

//---------------------------------------------------------------------

class SpreadContainer : public Container
{
public:

	SpreadContainer(Window* window, DWORD style);
	virtual ~SpreadContainer();

	virtual LRESULT onWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};

//---------------------------------------------------------------------

class ScrollContainer : public Container
{
public:

	ScrollContainer(Window* window, DWORD style);
	virtual ~ScrollContainer();

	void updateScrollBar(HWND hwndContainer);
	void scroll(HWND hwndContainer, int bar, WPARAM wParam);
	void recalcLayout(HWND hwndContainer);
	virtual void onResizeDockContainer(LPCRECT rc);
	virtual void onResizeFloatContainer();
	virtual BOOL onSetContainerPos(WINDOWPOS* wp);
	virtual LRESULT onWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual LRESULT onTargetWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};

//---------------------------------------------------------------------
