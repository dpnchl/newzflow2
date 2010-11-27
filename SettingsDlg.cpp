#include "stdafx.h"
#include "resource.h"
#include "SettingsDlg.h"
#include "Newzflow.h"
#include "Settings.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// CSettingsServerPage
//////////////////////////////////////////////////////////////////////////

CSettingsServerPage::CSettingsServerPage()
{
	CSettings* settings = CNewzflow::Instance()->settings;
	m_sHostname = settings->GetIni(_T("Server"), _T("Host"));
	m_nPort = min(65535, max(0, _ttoi(settings->GetIni(_T("Server"), _T("Port"), _T("119")))));
	m_sUser = settings->GetIni(_T("Server"), _T("User"));
	m_sPassword = settings->GetIni(_T("Server"), _T("Password"));
	m_nConnections = settings->GetConnections();
	m_nSpeedLimit = min(60000, max(0, settings->GetSpeedLimit() / 1024));
	m_bSpeedLimitActive = m_nSpeedLimit != 0;
}

CSettingsServerPage::~CSettingsServerPage()
{
}

int CSettingsServerPage::OnApply()
{
	CSettings* settings = CNewzflow::Instance()->settings;
	settings->SetIni(_T("Server"), _T("Host"), m_sHostname);
	settings->SetIni(_T("Server"), _T("Port"), m_nPort);
	settings->SetIni(_T("Server"), _T("User"), m_sUser);
	settings->SetIni(_T("Server"), _T("Password"), m_sPassword);
	settings->SetConnections(m_nConnections);
	settings->SetSpeedLimit(m_bSpeedLimitActive ? m_nSpeedLimit * 1024 : 0);
	CNewzflow::Instance()->OnServerSettingsChanged();

	return PSNRET_NOERROR;
}

BOOL CSettingsServerPage::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DoDataExchange(false);
	return TRUE;
}

BOOL CSettingsServerPage::OnKillActive()
{
	return DoDataExchange(true) ? PSNRET_NOERROR : PSNRET_INVALID;
}

LRESULT CSettingsServerPage::OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static bool bLocked = false;
	if(bLocked) return 0;
	bLocked = true;

	if(IsDlgButtonChecked(IDC_SPEEDLIMIT_CHECK)) {
		::EnableWindow(GetDlgItem(IDC_SPEEDLIMIT), TRUE);
	} else {
		::EnableWindow(GetDlgItem(IDC_SPEEDLIMIT), FALSE);
		::SetWindowText(GetDlgItem(IDC_SPEEDLIMIT), _T(""));
	}
	bLocked = false;
	SetModified(TRUE);
	return 0;
}

/*
void CSettingsServerPage::OnDataValidateError(UINT nCtrlID, BOOL bSave, _XData& data)
{
}

void CSettingsServerPage::OnDataExchangeError(UINT nCtrlID, BOOL bSave, _XData& data)
{
}
*/

// CSettingsSheet
//////////////////////////////////////////////////////////////////////////

CSettingsSheet::CSettingsSheet(HWND hWndParent /*= NULL*/)
: CPropertySheetImplEx<CSettingsSheet>(_T("Settings"), 0, hWndParent)
{
	m_psh.dwFlags |= PSH_NOCONTEXTHELP;

	AddPage(m_pgServer);
}
