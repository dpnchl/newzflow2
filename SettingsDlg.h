#pragma once

#include "PropertyEx.h"

// use this mix-in class instead of CWinDataExchange to enable balloon tips for data validation
// use CHAIN_MSG_MAP(CWinDataExchangeEx<T>) to forward necessary messages
template <class T>
class CWinDataExchangeEx : public CWinDataExchange<T>
{
public:
	BEGIN_MSG_MAP(CWinDataExchangeEx<T>)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
		COMMAND_CODE_HANDLER(EN_KILLFOCUS, OnEnKillFocus);
		MESSAGE_RANGE_HANDLER(WM_MOUSEFIRST, WM_MOUSELAST, OnMouseMessage)
	END_MSG_MAP()

	LRESULT OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
		if(m_tip.IsWindow())
			m_tip.TrackActivate(&m_ti, FALSE);
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnEnKillFocus(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
		if(m_tip.IsWindow())
			m_tip.TrackActivate(&m_ti, FALSE);
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		T* pT = static_cast<T*>(this);
		if(m_tip.IsWindow())
			m_tip.RelayEvent((LPMSG)pT->GetCurrentMessage());
		bHandled = FALSE;
		return 0;
	}

	void OnDataValidateError(UINT nCtrlID, BOOL bSave, _XData& data)
	{
		switch(data.nDataType) {
		case ddxDataInt: m_tiText.Format(_T("Please enter a number between %d and %d."), data.intData.nMin, data.intData.nMax); break;
		case ddxDataFloat: case ddxDataDouble: m_tiText.Format(_T("Please enter a number between %g and %g."), data.floatData.nMin, data.floatData.nMax); break;
		default: m_tiText.Format(_T("Please enter a valid value.")); break;
		}
		ShowDataError(nCtrlID);
	}

	void OnDataExchangeError(UINT nCtrlID, BOOL bSave)
	{
		m_tiText.Format(_T("Please enter a value."));
		ShowDataError(nCtrlID);
	}

private:
	void CreateToolTip()
	{
		T* pT = static_cast<T*>(this);
		m_tip.Create(*pT, NULL, NULL, TTS_NOPREFIX | TTS_NOFADE | TTS_BALLOON);
		m_tip.SetMaxTipWidth( 0 );
		m_tip.Activate( TRUE );
		CToolInfo ti(TTF_IDISHWND | TTF_SUBCLASS | TTF_TRACK, *pT, 0, 0, (LPTSTR)(LPCTSTR)m_tiText);
		m_tip.SetTitle(0, _T("Incorrect value"));
		m_tip.SetMaxTipWidth(300);
		m_tip.AddTool(&ti);
		m_ti = ti;
	}

	void ShowDataError(UINT nCtrlID) 
	{
		T* pT = static_cast<T*>(this);
		::MessageBeep((UINT)-1);
		::SetFocus(pT->GetDlgItem(nCtrlID));
		CEdit(pT->GetDlgItem(nCtrlID)).SetSelAll();
		CRect r;
		pT->GetDlgItem(nCtrlID).GetWindowRect(&r);
		if(!m_tip.IsWindow())
			CreateToolTip();
		m_ti.lpszText = (LPTSTR)(LPCTSTR)m_tiText;
		m_tip.TrackActivate(&m_ti, FALSE);
		m_tip.DelTool(&m_ti);
		m_tip.AddTool(&m_ti);
		m_tip.TrackPosition((pT->GetDlgItem(nCtrlID).GetExStyle() & WS_EX_RIGHT) ? (r.right - 8) : (r.left + 8), r.bottom - 8);
		m_tip.TrackActivate(&m_ti, TRUE);
	}

	CToolTipCtrl m_tip;
	TOOLINFO m_ti;
	CString m_tiText;
};

class CSettingsServerPage : public CPropertyPageImplEx<CSettingsServerPage>, public CWinDataExchangeEx<CSettingsServerPage>
{
public:
	enum { IDD = IDD_SETTINGS_SERVER };

	// Construction
	CSettingsServerPage();
	~CSettingsServerPage();

	// Maps
	BEGIN_MSG_MAP(CSettingsServerPage)
		CHAIN_MSG_MAP(CWinDataExchangeEx<CSettingsServerPage>) // must come first
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
	BEGIN_MSG_MAP(CSettingsDirectoriesPage)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
		COMMAND_HANDLER_EX(IDC_DOWNLOAD_DIR_BUTTON, BN_CLICKED, OnDownloadDirButton)
		COMMAND_HANDLER_EX(IDC_COMPLETED_DIR_BUTTON, BN_CLICKED, OnCompletedDirButton)
		COMMAND_HANDLER_EX(IDC_WATCH_DIR_BUTTON, BN_CLICKED, OnWatchDirButton)
		COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)
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
