#include "stdafx.h"
#include "resource.h"
#include "SettingsDlg.h"
#include "Newzflow.h"
#include "Settings.h"
#include "Util.h"

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

LRESULT CSettingsServerPage::OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
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

	bHandled = FALSE;
	return 0;
}

// CSettingsDirectoriesPage
//////////////////////////////////////////////////////////////////////////

CSettingsDirectoriesPage::CSettingsDirectoriesPage()
{
	CSettings* settings = CNewzflow::Instance()->settings;
	m_bDownloadDir = settings->GetIni(_T("Directories"), _T("UseDownload"), _T("0")) != _T("0");
	m_sDownloadDir = settings->GetIni(_T("Directories"), _T("Download"));
	m_bCompletedDir = settings->GetIni(_T("Directories"), _T("UseCompleted"), _T("0")) != _T("0");
	m_sCompletedDir = settings->GetIni(_T("Directories"), _T("Completed"));
	m_bWatchDir = settings->GetIni(_T("Directories"), _T("UseWatch"), _T("0")) != _T("0");
	m_bWatchDirDelete = settings->GetIni(_T("Directories"), _T("DeleteWatch"), _T("0")) != _T("0");
	m_sWatchDir = settings->GetIni(_T("Directories"), _T("Watch"));
}

CSettingsDirectoriesPage::~CSettingsDirectoriesPage()
{
}

BOOL CSettingsDirectoriesPage::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DoDataExchange(false);
	return TRUE;
}

LRESULT CSettingsDirectoriesPage::OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
	static bool bLocked = false;
	if(bLocked) return 0;
	bLocked = true;

	GetDlgItem(IDC_DOWNLOAD_DIR).EnableWindow(IsDlgButtonChecked(IDC_DOWNLOAD_DIR_CHECK));
	GetDlgItem(IDC_DOWNLOAD_DIR_BUTTON).EnableWindow(IsDlgButtonChecked(IDC_DOWNLOAD_DIR_CHECK));

	GetDlgItem(IDC_COMPLETED_DIR).EnableWindow(IsDlgButtonChecked(IDC_COMPLETED_DIR_CHECK));
	GetDlgItem(IDC_COMPLETED_DIR_BUTTON).EnableWindow(IsDlgButtonChecked(IDC_COMPLETED_DIR_CHECK));

	GetDlgItem(IDC_WATCH_DIR_DELETE).EnableWindow(IsDlgButtonChecked(IDC_WATCH_DIR_CHECK));
	GetDlgItem(IDC_WATCH_DIR).EnableWindow(IsDlgButtonChecked(IDC_WATCH_DIR_CHECK));
	GetDlgItem(IDC_WATCH_DIR_BUTTON).EnableWindow(IsDlgButtonChecked(IDC_WATCH_DIR_CHECK));

	bLocked = false;
	SetModified(TRUE);

	bHandled = FALSE;

	return 0;
}

int CSettingsDirectoriesPage::OnApply()
{
	CSettings* settings = CNewzflow::Instance()->settings;
	settings->SetIni(_T("Directories"), _T("UseDownload"), m_bDownloadDir);
	settings->SetIni(_T("Directories"), _T("Download"), m_sDownloadDir);
	settings->SetIni(_T("Directories"), _T("UseCompleted"), m_bCompletedDir);
	settings->SetIni(_T("Directories"), _T("Completed"), m_sCompletedDir);
	settings->SetIni(_T("Directories"), _T("UseWatch"), m_bWatchDir);
	settings->SetIni(_T("Directories"), _T("DeleteWatch"), m_bWatchDirDelete);
	settings->SetIni(_T("Directories"), _T("Watch"), m_sWatchDir);
	return PSNRET_NOERROR;
}

BOOL CSettingsDirectoriesPage::OnKillActive()
{
	if(DoDataExchange(true)) {
		if(m_bDownloadDir) {
			int result = Util::TestCreateDirectory(m_sDownloadDir);
			if(result != ERROR_SUCCESS && result != ERROR_ALREADY_EXISTS && result != ERROR_FILE_EXISTS) {
				OnDataCustomError(IDC_DOWNLOAD_DIR, Util::GetErrorMessage(result));
				return PSNRET_INVALID;
			}
		}
		if(m_bCompletedDir) {
			int result = Util::TestCreateDirectory(m_sCompletedDir);
			if(result != ERROR_SUCCESS && result != ERROR_ALREADY_EXISTS && result != ERROR_FILE_EXISTS) {
				OnDataCustomError(IDC_COMPLETED_DIR, Util::GetErrorMessage(result));
				return PSNRET_INVALID;
			}
		}
		if(m_bWatchDir) {
			int result = Util::TestCreateDirectory(m_sWatchDir);
			if(result != ERROR_SUCCESS && result != ERROR_ALREADY_EXISTS && result != ERROR_FILE_EXISTS) {
				OnDataCustomError(IDC_WATCH_DIR, Util::GetErrorMessage(result));
				return PSNRET_INVALID;
			}
		}
		return PSNRET_NOERROR;
	} else {
		return PSNRET_INVALID;
	}
}

void CSettingsDirectoriesPage::OnDownloadDirButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/)
{
	CString sDir;
	GetDlgItemText(IDC_DOWNLOAD_DIR, sDir);
	SetDlgItemText(IDC_DOWNLOAD_DIR, Util::BrowseForFolder(*this, NULL, sDir));
}

void CSettingsDirectoriesPage::OnCompletedDirButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/)
{
	CString sDir;
	GetDlgItemText(IDC_COMPLETED_DIR, sDir);
	SetDlgItemText(IDC_COMPLETED_DIR, Util::BrowseForFolder(*this, NULL, sDir));
}

void CSettingsDirectoriesPage::OnWatchDirButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/)
{
	CString sDir;
	GetDlgItemText(IDC_WATCH_DIR, sDir);
	SetDlgItemText(IDC_WATCH_DIR, Util::BrowseForFolder(*this, NULL, sDir));
}

// CSettingsSheet
//////////////////////////////////////////////////////////////////////////

// CSettingsSheet uses a slightly weird Creation/Deletion method.
// We need to consider 2 requirements:
// - Create a new CSettingsSheet instance every time the settings dialog wants to be shown (Multiple calls to Create() ASSERT in CPropertySheetImpl)
// - don't delete a CSettingsSheet instance in OnFinalMessage() because there may still be a modal dialog / shell dialog open when sheet receives WM_DESTROY
//   and if we delete the instance at this point, we get a heap corruption because the message loop is still running for this sheet

/*static*/ CSettingsSheet* CSettingsSheet::s_pCurInstance = NULL;
/*static*/ CSettingsSheet* CSettingsSheet::s_pLastInstance = NULL;

CSettingsSheet::CSettingsSheet(HWND hWndParent /*= NULL*/)
: CPropertySheetImplEx<CSettingsSheet>(_T("Settings"), 0, hWndParent)
{
	m_psh.dwFlags |= PSH_NOCONTEXTHELP;

	AddPage(m_pgServer);
	AddPage(m_pgDirectories);
}

void CSettingsSheet::OnFinalMessage(HWND)
{
	s_pCurInstance = NULL;
	s_pLastInstance = this;
}

void CSettingsSheet::Show(HWND hWndParent)
{
	if(s_pCurInstance) {
		if(s_pCurInstance->IsWindow()) {
			s_pCurInstance->BringWindowToTop();
			return;
		}
	} else {
		Cleanup();
		s_pCurInstance = new CSettingsSheet;
		s_pCurInstance->Create(hWndParent);
	}
}

// is also called from ~CMainFrame()
void CSettingsSheet::Cleanup()
{
	if(s_pLastInstance) {
		delete s_pLastInstance;
		s_pLastInstance = NULL;
	}
}
