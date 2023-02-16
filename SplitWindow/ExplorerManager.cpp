#include "pch.h"
#include "SplitWindow.h"

//---------------------------------------------------------------------

ObjectExplorer::ObjectExplorer(HWND hwnd)
{
	TCHAR path[MAX_PATH] = {};
	::GetModuleFileName(g_instance, path, MAX_PATH);
	::PathRemoveFileSpec(path);
	::PathAppend(path, _T("ObjectExplorer\\ObjectExplorer.exe"));
	MY_TRACE_TSTR(path);

	TCHAR args[MAX_PATH] = {};
	::StringCbPrintf(args, sizeof(args), _T("0x%08p"), hwnd);
	MY_TRACE_TSTR(args);

	STARTUPINFO si = { sizeof(si) };
	if (!::CreateProcess(
		path,           // No module name (use command line)
		args,           // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi))         // Pointer to PROCESS_INFORMATION structur
	{
		MY_TRACE(_T("::CreateProcess() failed.\n"));
	}
}

ObjectExplorer::~ObjectExplorer()
{
	::CloseHandle(pi.hThread);
	::CloseHandle(pi.hProcess);
}

//---------------------------------------------------------------------

ObjectExplorer* ExplorerManager::getObjectExplorer(HWND explorer)
{
	return (ObjectExplorer*)::GetProp(explorer, _T("SplitWindow.ObjectExplorer"));
}

BOOL ExplorerManager::sendExitMessage(HWND explorer)
{
	MY_TRACE(_T("ExplorerManager::sendExitMessage(0x%08X)\n"), explorer);

	ObjectExplorer* objectExplorer =
		g_explorerManager.getObjectExplorer(explorer);

	if (!objectExplorer) return FALSE;

	::SendMessage(objectExplorer->dialog,
		WM_AVIUTL_OBJECT_EXPLORER_EXIT, 0, 0);

	return TRUE;
}

BOOL ExplorerManager::sendExitMessage()
{
	MY_TRACE(_T("ExplorerManager::sendExitMessage()\n"));

	for (auto explorer : m_array)
	{
		if (sendExitMessage(explorer))
			return TRUE;
	}

	return FALSE;
}

void ExplorerManager::updateExplorerMenu()
{
	// すべての項目を削除する。
	while (::DeleteMenu(g_explorerMenu, 0, MF_BYPOSITION)){}

	int c = (int)m_array.size();
	for (int i = 0; i < c; i++)
	{
		HWND explorer = m_array[i];

		MENUITEMINFO mii = { sizeof(mii) };
		mii.fMask = MIIM_STRING | MIIM_DATA | MIIM_ID | MIIM_STATE;

		TCHAR windowName[MAX_PATH] = {};
		::GetWindowText(explorer, windowName, MAX_PATH);

		if (::IsWindowVisible(explorer))
			mii.fState = MFS_CHECKED;
		else
			mii.fState = MFS_UNCHECKED;

		mii.wID = CommandID::EXPLORER_BEGIN + i;
		mii.dwItemData = (DWORD)explorer;
		mii.dwTypeData = windowName;

		::InsertMenuItem(g_explorerMenu, i, TRUE, &mii);
	}
}

void ExplorerManager::executeExplorerMenu(UINT id)
{
	MENUITEMINFO mii = { sizeof(mii) };
	mii.fMask = MIIM_DATA;
	::GetMenuItemInfo(g_explorerMenu, id, FALSE, &mii);

	HWND explorer = (HWND)mii.dwItemData;
	if (explorer)
		::SendMessage(explorer, WM_CLOSE, 0, 0);
}

//---------------------------------------------------------------------

void ExplorerManager::insert(HWND explorer)
{
	ObjectExplorer* objectExplorer = new ObjectExplorer(explorer);

	::SetProp(explorer, _T("SplitWindow.ObjectExplorer"), objectExplorer);

	// コレクションに追加する。
	m_array.push_back(explorer);
}

void ExplorerManager::erase(HWND explorer)
{
	// コレクションから削除する。
	auto it = std::find(m_array.begin(), m_array.end(), explorer);
	if (it != m_array.end()) m_array.erase(it);

	ObjectExplorer* objectExplorer = (ObjectExplorer*)
		::RemoveProp(explorer, _T("SplitWindow.ObjectExplorer"));

	delete objectExplorer;
}

void ExplorerManager::clear()
{
	// ::DestroyWindow(explorer) でコレクションの要素が減るのでコレクションをコピーしておく。
	auto explorerArray = m_array;

	// 全てのコロニーを削除する。
	for (auto explorer : explorerArray)
		::DestroyWindow(explorer);

	// 念のためクリアする。
	m_array.clear();
}

//---------------------------------------------------------------------
