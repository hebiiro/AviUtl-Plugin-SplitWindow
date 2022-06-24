#pragma once

#include "Resource.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

int showConfigDialog(HWND hwnd);

//---------------------------------------------------------------------

class ConfigDialog : public Dialog
{
public:
	ConfigDialog(HWND hwnd);

	virtual void onOK();
	virtual void onCancel();
	virtual INT_PTR onDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};

//---------------------------------------------------------------------
