#pragma once

//---------------------------------------------------------------------

class Window
{
public:

	HWND m_hwnd = 0;
	ContainerPtr m_dockContainer;
	ContainerPtr m_floatContainer;
	WNDPROC m_targetWndProc = 0;
	Pane* m_pane = 0;
	_bstr_t m_name = L"";

	virtual void init(HWND hwnd);
	virtual Container* onCreateDockContainer();
	virtual Container* onCreateFloatContainer();
	virtual DWORD onGetTargetNewStyle();
	virtual void onSetTargetWindowPos(LPRECT rc);
	virtual LRESULT onContainerWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual LRESULT onTargetWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	void showTargetWindow();
	void dockWindow(LPCRECT rc);
	void floatWindow();

	virtual void resizeDockContainer(LPCRECT rc);
	virtual void resizeFloatContainer();

	static LRESULT CALLBACK targetWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	static Window* getWindow(HWND hwnd);
	static void setWindow(HWND hwnd, Window* window);
};

//---------------------------------------------------------------------

class AviUtlWindow : public Window
{
public:

	virtual void init(HWND hwnd);
	virtual LRESULT onContainerWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual LRESULT onTargetWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};

//---------------------------------------------------------------------

class ExEditWindow : public Window
{
public:

	virtual void init(HWND hwnd);
	virtual DWORD onGetTargetNewStyle();
	virtual void onSetTargetWindowPos(LPRECT rc);
	virtual LRESULT onContainerWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual LRESULT onTargetWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};

//---------------------------------------------------------------------

class SettingDialog : public Window
{
public:

	virtual void init(HWND hwnd);
	virtual Container* onCreateDockContainer();
	virtual Container* onCreateFloatContainer();
	virtual LRESULT onContainerWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual LRESULT onTargetWndProc(Container* container, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};

//---------------------------------------------------------------------
