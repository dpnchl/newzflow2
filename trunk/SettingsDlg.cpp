#include "stdafx.h"
#include "resource.h"
#include "SettingsDlg.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// CSettingsServerPage
//////////////////////////////////////////////////////////////////////////

CSettingsServerPage::CSettingsServerPage()
{
}

CSettingsServerPage::~CSettingsServerPage()
{
}

int CSettingsServerPage::OnApply()
{
	return DoDataExchange(true) ? PSNRET_NOERROR : PSNRET_INVALID;
}

BOOL CSettingsServerPage::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	return TRUE;
}

CSettingsSheet::CSettingsSheet(HWND hWndParent /*= NULL*/)
: CPropertySheetImplEx<CSettingsSheet>(_T("Settings"), 0, hWndParent)
{
	m_psh.dwFlags |= PSH_NOCONTEXTHELP;

	AddPage(m_pgServer);
	AddPage(m_pgAbout);
}
