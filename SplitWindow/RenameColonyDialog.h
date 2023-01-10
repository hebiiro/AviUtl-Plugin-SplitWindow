#pragma once

#include "Resource.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

class RenameColonyDialog : public Dialog
{
public:
	ShuttleManager* m_shuttleManager = 0;
	Shuttle* m_shuttle = 0;

public:
	RenameColonyDialog(HWND parent);

	virtual int doModal();
	virtual void onOK();
	virtual void onCancel();
};

//---------------------------------------------------------------------
