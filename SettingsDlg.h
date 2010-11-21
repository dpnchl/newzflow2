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
		CHAIN_MSG_MAP(CPropertyPageImplEx<CSettingsServerPage>)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CSettingsServerPage)
		//DDX_RADIO(IDC_BLUE, m_nColor)
		//DDX_RADIO(IDC_ALYSON, m_nPicture)
	END_DDX_MAP()

	// Message handlers
	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);

	// Property page notification handlers
	int OnApply();

	// DDX variables
	int m_nColor, m_nPicture;
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
	CSettingsServerPage           m_pgServer;
	CPropertyPageEx<IDD_ABOUTBOX> m_pgAbout;
};