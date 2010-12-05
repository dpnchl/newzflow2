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

	int GetSpeedLimit();
	void SetSpeedLimit(int limit);

	int GetConnections();
	void SetConnections(int connections);

	void GetListViewColumns(const CString& name, CListViewCtrl lv, int* columnVisibility, int maxColumns);
	void SetListViewColumns(const CString& name, CListViewCtrl lv, int* columnsVisibility, int maxColumns);

	CString GetDownloadDir();
	CString GetWatchDir();
	bool GetDeleteWatch();

	CString GetIni(LPCTSTR section, LPCTSTR key, LPCTSTR def = NULL);
	void SetIni(LPCTSTR section, LPCTSTR key, LPCTSTR value);
	void SetIni(LPCTSTR section, LPCTSTR key, int value);

	const CString& GetAppDataDir();

	CAtlArray<CString> downloadDirHistory;

protected:
	CString m_appData, m_ini;
};