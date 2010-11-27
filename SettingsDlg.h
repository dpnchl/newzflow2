#pragma once

#include "PropertyEx.h"

class CSettingsServerPage : public CPropertyPageImplEx<CSettingsServerPage>, public CWinDataExchange<CSettingsServerPage>
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

	void OnDataValidateError(UINT nCtrlID, BOOL bSave, _XData& data);
	//void OnDataExchangeError(UINT nCtrlID, BOOL bSave, _XData& data);

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

class CSettingsDirectoriesPage : public CPropertyPageImplEx<CSettingsDirectoriesPage>, public CWinDataExchange<CSettingsDirectoriesPage>
{
public:
	enum { IDD = IDD_SETTINGS_DIRECTORIES };

	// Construction
	CSettingsDirectoriesPage();
	~CSettingsDirectoriesPage();

	// Maps
	BEGIN_MSG_MAP_EX(CSettingsDirectoriesPage)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
		COMMAND_HANDLER_EX(IDC_DOWNLOAD_DIR_BUTTON, BN_CLICKED, OnDownloadDirButton)
		COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)
		CHAIN_MSG_MAP(CPropertyPageImplEx<CSettingsDirectoriesPage>)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CSettingsDirectoriesPage)
/*		DDX_TEXT(IDC_HOSTNAME, m_sHostname)
		DDX_INT_RANGE(IDC_PORT, m_nPort, 0, 65535)
		DDX_TEXT(IDC_USER, m_sUser)
		DDX_TEXT(IDC_PASSWORD, m_sPassword)
		DDX_INT_RANGE(IDC_CONNECTIONS, m_nConnections, 0, 100)
		DDX_CHECK(IDC_SPEEDLIMIT_CHECK, m_bSpeedLimitActive)
		if(m_bSpeedLimitActive)
			DDX_INT_RANGE(IDC_SPEEDLIMIT, m_nSpeedLimit, 1, 60000)
*/
	END_DDX_MAP()

	// Message handlers
	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	void OnDownloadDirButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/);

	//void OnDataValidateError(UINT nCtrlID, BOOL bSave, _XData& data);
	//void OnDataExchangeError(UINT nCtrlID, BOOL bSave, _XData& data);

	// Property page notification handlers
	int OnApply();
	BOOL OnKillActive();

	// DDX variables
/*	CString m_sHostname;
	int m_nPort;
	CString m_sUser;
	CString m_sPassword;
	int m_nConnections;
	int m_bSpeedLimitActive;
	int m_nSpeedLimit;
*/
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
};
