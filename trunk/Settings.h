#pragma once

class CSettings
{
public:
	CSettings();
	~CSettings();

	CRect GetWindowPos(int& showCmd);
	void SetWindowPos(const CRect& r, int showCmd);

	int GetVertSplitPos();
	int GetHorzSplitPos();
	void SetVertSplitPos(int split);
	void SetHorzSplitPos(int split);

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
	const CString& GetProgramDir();
	void GetListViewSort(const CString& name, int& sortColumn, bool& sortAsc);
	void SetListViewSort(const CString& name, int sortColumn, bool sortAsc);
	CAtlArray<CString> downloadDirHistory;

	enum EMovieQuality {
		mqBD,
		mqMKV1080p,
		mqMKV720p,
		mqWMV1080p,
		mqWMV720p,
		mqDVD,
		mqXvid,

		mqMax, // marker for end-of-enum
	};
	static CString GetMovieQualityName(int q);

protected:
	CString m_appData; // %appdata%\Newzflow
	CString m_ini; // %appdata%\Newzflow\Newzflow.ini
	CString m_programDir; // programDir (no trailing backslash)
};