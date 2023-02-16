#pragma once

//---------------------------------------------------------------------

struct ObjectExplorer
{
	PROCESS_INFORMATION pi = {};
	HWND dialog = 0;
	HWND browser = 0;

	ObjectExplorer(HWND hwnd);
	~ObjectExplorer();
};

//---------------------------------------------------------------------

class ExplorerManager
{
public:

	std::vector<HWND> m_array;

public:

	ObjectExplorer* getObjectExplorer(HWND explorer);
	BOOL sendExitMessage(HWND explorer);
	BOOL sendExitMessage();
	void updateExplorerMenu();
	void executeExplorerMenu(UINT id);

	void insert(HWND explorer);
	void erase(HWND explorer);
	void clear();
};

//---------------------------------------------------------------------
