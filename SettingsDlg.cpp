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
	m_nConnections = min(100, max(0, _ttoi(settings->GetIni(_T("Server"), _T("Connections"), _T("10")))));
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
	settings->SetIni(_T("Server"), _T("Connections"), m_nConnections);
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
