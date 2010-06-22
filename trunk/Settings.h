#pragma once

class CSettings
{
public:
	CSettings();
	~CSettings();

	CRect GetWindowPos(int& showCmd);
	void SetWindowPos(const CRect& r, int showCmd);

	int GetSplitPos();
	void SetSplitPos(int split);

	void GetListViewColumns(const CString& name, CListViewCtrl lv);
	void SetListViewColumns(const CString& name, CListViewCtrl lv);

	const CString& GetAppDataDir();

protected:
	CString m_appData, m_ini;
};