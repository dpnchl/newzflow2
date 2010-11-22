#pragma once

#include "PropertyEx.h"

class CSettingsServerPage :
	public CPropertyPageImplEx<CSettingsServerPage>,
	public CWinDataExchange<CSettingsServerPage>
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
		CHAIN_MSG_MAP(CPropertyPageImplEx<CSettingsServerPage>)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CSettingsServerPage)
		DDX_TEXT(IDC_HOSTNAME, m_sHostname)
		DDX_INT_RANGE(IDC_PORT, m_nPort, 0, 65535)
		DDX_TEXT(IDC_USER, m_sUser)
		DDX_TEXT(IDC_PASSWORD, m_sPassword)
		DDX_INT_RANGE(IDC_CONNECTIONS, m_nConnections, 0, 100)
	END_DDX_MAP()

	// Message handlers
	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

/* // TODO
	void OnDataValidateError ( UINT nCtrlID, BOOL bSave, _XData& data )
	{
	CString sMsg;
	 
		sMsg.Format ( _T("Enter a number between %d and %d"),
					  data.intData.nMin, data.intData.nMax );
	 
		MessageBox ( sMsg, _T("ControlMania2"), MB_ICONEXCLAMATION );
	 
		GotoDlgCtrl ( GetDlgItem(nCtrlID) );
	}
*/

	// Property page notification handlers
	int OnApply();
	BOOL OnKillActive();

	// DDX variables
	CString m_sHostname;
	int m_nPort;
	CString m_sUser;
	CString m_sPassword;
	int m_nConnections;
};

class CSettingsSheet : public CPropertySheetImplEx<CSettingsSheet>
{
public:
	// Construction
	CSettingsSheet(HWND hWndParent = NULL);

	// Maps
	BEGIN_MSG_MAP(CSettingsSheet)
		CHAIN_MSG_MAP(CPropertySheetImplEx<CSettingsSheet>)
	END_MSG_MAP()

	// Property pages
	CSettingsServerPage m_pgServer;
};