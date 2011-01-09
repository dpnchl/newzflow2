#pragma once

#include "DialogEx.h"

class CSettingsServerPage : public CPropertyPageImplEx<CSettingsServerPage>, public CWinDataExchangeEx<CSettingsServerPage>
{
public:
	enum { IDD = IDD_SETTINGS_SERVER };

	// Construction
	CSettingsServerPage();
	~CSettingsServerPage();

	// Maps
	BEGIN_MSG_MAP(CSettingsServerPage)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
		COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)
		CHAIN_MSG_MAP(CWinDataExchangeEx<CSettingsServerPage>)
		CHAIN_MSG_MAP(CPropertyPageImplEx<CSettingsServerPage>)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CSettingsServerPage)
		DDX_TEXT(IDC_HOSTNAME, m_sHostname)
		DDX_INT_RANGE(IDC_PORT, m_nPort, 0, 65535)
		DDX_TEXT(IDC_USER, m_sUser)
		DDX_TEXT(IDC_PASSWORD, m_sPassword)
		DDX_INT_RANGE(IDC_CONNECTIONS, m_nConnections, 0, 100)
		DDX_CHECK(IDC_SPEEDLIMIT_CHECK, m_bSpeedLimitActive)
		if(m_bSpeedLimitActive)
			DDX_INT_RANGE(IDC_SPEEDLIMIT, m_nSpeedLimit, 1, 60000)
	END_DDX_MAP()

	// Message handlers
	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// Property page notification handlers
	int OnApply();
	BOOL OnKillActive();

	// DDX variables
	CString m_sHostname;
	int m_nPort;
	CString m_sUser;
	CString m_sPassword;
	int m_nConnections;
	int m_bSpeedLimitActive;
	int m_nSpeedLimit;
};

class CSettingsDirectoriesPage : public CPropertyPageImplEx<CSettingsDirectoriesPage>, public CWinDataExchangeEx<CSettingsDirectoriesPage>
{
public:
	enum { IDD = IDD_SETTINGS_DIRECTORIES };

	// Construction
	CSettingsDirectoriesPage();
	~CSettingsDirectoriesPage();

	// Maps
	BEGIN_MSG_MAP(CSettingsDirectoriesPage)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
		COMMAND_HANDLER_EX(IDC_DOWNLOAD_DIR_BUTTON, BN_CLICKED, OnDownloadDirButton)
		COMMAND_HANDLER_EX(IDC_COMPLETED_DIR_BUTTON, BN_CLICKED, OnCompletedDirButton)
		COMMAND_HANDLER_EX(IDC_WATCH_DIR_BUTTON, BN_CLICKED, OnWatchDirButton)
		COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)
		CHAIN_MSG_MAP(CWinDataExchangeEx<CSettingsDirectoriesPage>)
		CHAIN_MSG_MAP(CPropertyPageImplEx<CSettingsDirectoriesPage>)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CSettingsDirectoriesPage)
		DDX_CHECK(IDC_DOWNLOAD_DIR_CHECK, m_bDownloadDir)
		DDX_TEXT(IDC_DOWNLOAD_DIR, m_sDownloadDir)
		DDX_CHECK(IDC_COMPLETED_DIR_CHECK, m_bCompletedDir)
		DDX_TEXT(IDC_COMPLETED_DIR, m_sCompletedDir)
		DDX_CHECK(IDC_WATCH_DIR_CHECK, m_bWatchDir)
		DDX_CHECK(IDC_WATCH_DIR_DELETE, m_bWatchDirDelete)
		DDX_TEXT(IDC_WATCH_DIR, m_sWatchDir)
	END_DDX_MAP()

	// Message handlers
	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	void OnDownloadDirButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/);
	void OnCompletedDirButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/);
	void OnWatchDirButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/);

	//void OnDataValidateError(UINT nCtrlID, BOOL bSave, _XData& data);
	//void OnDataExchangeError(UINT nCtrlID, BOOL bSave, _XData& data);

	// Property page notification handlers
	int OnApply();
	BOOL OnKillActive();

	// DDX variables
	bool m_bDownloadDir;
	CString m_sDownloadDir;
	bool m_bCompletedDir;
	CString m_sCompletedDir;
	bool m_bWatchDir, m_bWatchDirDelete;
	CString m_sWatchDir;
};

class CSettingsProviderPage : public CPropertyPageImplEx<CSettingsProviderPage>, public CWinDataExchangeEx<CSettingsProviderPage>
{
public:
	enum { IDD = IDD_SETTINGS_PROVIDER };

	// Construction
	CSettingsProviderPage();
	~CSettingsProviderPage();

	// Maps
	BEGIN_MSG_MAP(CSettingsProviderPage)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
		COMMAND_HANDLER(IDC_NZBMATRIX_CHECK, BN_CLICKED, OnCheck)
		COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)
		CHAIN_MSG_MAP(CWinDataExchangeEx<CSettingsProviderPage>)
		CHAIN_MSG_MAP(CPropertyPageImplEx<CSettingsProviderPage>)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CSettingsProviderPage)
		DDX_TEXT(IDC_NZBMATRIX_USER, m_sNzbMatrixUser)
		DDX_TEXT(IDC_NZBMATRIX_APIKEY, m_sNzbMatrixApiKey)
		DDX_CHECK(IDC_NZBMATRIX_CHECK, m_bNzbMatrix)
	END_DDX_MAP()

	// Message handlers
	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnCheck(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// Property page notification handlers
	int OnApply();
	BOOL OnKillActive();

	// DDX variables
	CString m_sNzbMatrixUser, m_sNzbMatrixApiKey;
	bool m_bNzbMatrix;
};

class CSettingsMoviesPage : public CPropertyPageImplEx<CSettingsMoviesPage>, public CWinDataExchangeEx<CSettingsMoviesPage>
{
public:
	enum { IDD = IDD_SETTINGS_MOVIES };

	// Construction
	CSettingsMoviesPage();
	~CSettingsMoviesPage();

	// Maps
	BEGIN_MSG_MAP(CSettingsMoviesPage)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
		COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)
		CHAIN_MSG_MAP(CWinDataExchangeEx<CSettingsMoviesPage>)
		CHAIN_MSG_MAP(CPropertyPageImplEx<CSettingsMoviesPage>)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CSettingsMoviesPage)
		DDX_CONTROL_HANDLE(IDC_LIST, m_List)
	END_DDX_MAP()

	// Message handlers
	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// Property page notification handlers
	int OnApply();
	BOOL OnKillActive();

	// DDX variables
	CListViewCtrl m_List;
};

class CSettingsSheet : public CPropertySheetImplEx<CSettingsSheet>
{
public:
	// Construction
	CSettingsSheet(HWND hWndParent = NULL);

protected:
	static CSettingsSheet* s_pCurInstance;
	static CSettingsSheet* s_pLastInstance;

public:
	static void Show(HWND hWndParent); // call to show settings dialog
	static void Cleanup(); // call in ~CMainFrame to clean up memory

	// Maps
	BEGIN_MSG_MAP(CSettingsSheet)
		CHAIN_MSG_MAP(CPropertySheetImplEx<CSettingsSheet>)
	END_MSG_MAP()

	virtual void OnFinalMessage(HWND);

	// Property pages
	CSettingsServerPage m_pgServer;
	CSettingsDirectoriesPage m_pgDirectories;
	CSettingsProviderPage m_pgProvider;
	CSettingsMoviesPage m_pgMovies;
};
