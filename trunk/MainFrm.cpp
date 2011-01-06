// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "Version.h"

#include "aboutdlg.h"
#include "MainFrm.h"

#include "Newzflow.h"
#include "Settings.h"
#include "NntpSocket.h"
#include "Util.h"
#include "SettingsDlg.h"
#include "DropTarget.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

const UINT CMainFrame::s_msgTaskbarButtonCreated = RegisterWindowMessage(_T("TaskbarButtonCreated"));

CMainFrame::CMainFrame()
{
	m_pTopView = NULL;
}

CMainFrame::~CMainFrame()
{
	CSettingsSheet::Cleanup();
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;

	return m_NzbView.PreTranslateMessage(pMsg); // ??? why ???
}

BOOL CMainFrame::OnIdle()
{
	CString s;
	s.Format(_T("Connections: %d"), CNewzflow::Instance()->settings->GetConnections()); UISetText(3, s);
	s.Format(_T("Max. Speed: %s"), CNntpSocket::speedLimiter.GetLimit() > 0 ? Util::FormatSpeed(CNntpSocket::speedLimiter.GetLimit()) : _T("Unlimited")); UISetText(4, s);

	UIUpdateToolBar();
	UIUpdateStatusBar();
	return FALSE;
}

void CMainFrame::UpdateLayout(BOOL bResizeBars)
{
	if(!IsWindowVisible())
		return;

	CRect rect;
	GetClientRect(rect);

	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	CRect rtree(rect);
	int treeWidth = m_horzSplitX;
	rtree.right = treeWidth - 2;
	rect.left = treeWidth + 2;
	CRect r1(rect), r2(rect);
	m_vertSplitYReal = max<int>(100, rect.bottom - max<int>(100, m_vertSplitY));
	r1.bottom = m_vertSplitYReal - 2;
	r2.top = m_vertSplitYReal + 2;

	HDWP hdwp = BeginDeferWindowPos(3);
	if(m_TreeView.IsWindow())
		m_TreeView.DeferWindowPos(hdwp, NULL, rtree.left, rtree.top, rtree.Width(), rtree.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
	if(m_pTopView && m_pTopView->IsWindow())
		m_pTopView->DeferWindowPos(hdwp, NULL, r1.left, r1.top, r1.Width(), r1.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
	if(m_TabView[1].IsWindow())
		m_TabView[1].DeferWindowPos(hdwp, NULL, r2.left, r2.top, r2.Width(), r2.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
	EndDeferWindowPos(hdwp);
}

LRESULT CMainFrame::OnSetCursor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// handle cursor for splitter
	const DWORD pos = GetMessagePos();
	CPoint pt(GET_X_LPARAM(pos), GET_Y_LPARAM(pos));
	ScreenToClient(&pt);

	if(pt.x >= m_horzSplitX - 3 && pt.x <= m_horzSplitX + 3) {
		SetCursor(LoadCursor(NULL, IDC_SIZEWE));
		return 1;
	}

	if(pt.y >= m_vertSplitYReal - 3 && pt.y <= m_vertSplitYReal + 3) {
		SetCursor(LoadCursor(NULL, IDC_SIZENS));
		return 1;
	}

	bHandled = FALSE;
	return 0;
}

LRESULT CMainFrame::OnMouseMove(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// handle splitter drag
	if((wParam & MK_LBUTTON) != 0 && ::GetCapture() == *this) {
		lParam = GetMessagePos();
		CPoint ptMouseCur(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		CRect r; GetClientRect(r);
		if(m_iDragMode == 0) {
			m_horzSplitX = max<int>(100, min<int>(r.Width() - 200, m_ptSplit.x + (ptMouseCur.x - m_ptMouse.x)));
			SetCursor(LoadCursor(NULL, IDC_SIZEWE));
		} else if(m_iDragMode == 1) {
			m_vertSplitY = max<int>(100, min<int>(r.Height() - 100, m_ptSplit.y - (ptMouseCur.y - m_ptMouse.y)));
			SetCursor(LoadCursor(NULL, IDC_SIZENS));
		}
		UpdateLayout();
		return 1;
	}
	bHandled = FALSE;
	return 0;
}

LRESULT CMainFrame::OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
	// handle splitter drag
	lParam = GetMessagePos();
	CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	ScreenToClient(&pt);
	if(pt.x >= m_horzSplitX - 3 && pt.x <= m_horzSplitX + 3) {
		m_iDragMode = 0;
		SetCapture();
		SetCursor(LoadCursor(NULL, IDC_SIZEWE));
		m_ptMouse = CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		CRect r; GetClientRect(r);
		m_ptSplit = CPoint(m_horzSplitX, 0);
		return 1;
	}
	if(pt.y >= m_vertSplitYReal - 3 && pt.y <= m_vertSplitYReal + 3) {
		m_iDragMode = 1;
		SetCapture();
		SetCursor(LoadCursor(NULL, IDC_SIZENS));
		m_ptMouse = CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		CRect r; GetClientRect(r);
		m_ptSplit = CPoint(0, max(100, min(r.Height() - 100, m_vertSplitY)));
		return 1;
	}
	bHandled = FALSE;
	return 0;
}

LRESULT CMainFrame::OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	ReleaseCapture();
	return 1;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// allow TaskbarButtonCreated message to be sent to us if we run elevated
	// we don't call this as of yet, because it only works on Windows 7, and we're not running elevated anyways
	//CHANGEFILTERSTRUCT cfs = { sizeof(CHANGEFILTERSTRUCT) };
	//ChangeWindowMessageFilterEx(m_hWnd, s_msgTaskbarButtonCreated, MSGFLT_ALLOW, &cfs);

	HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);

	if(!m_ToolBarImageList.Load(CNewzflow::Instance()->settings->GetAppDataDir() + _T("toolbar.bmp"), 24, 24))
		m_ToolBarImageList.LoadFromResource(IDB_TOOLBAR, 24, 24);
	m_ToolBarImageList.Set(hWndToolBar);

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndToolBar);

	m_hWndStatusBar = m_statusBar.Create(*this);
	UIAddStatusBar(m_hWndStatusBar);
	int nPanes[] = { ID_DEFAULT_PANE, IDR_PAUSE, IDR_SHUTDOWN, IDR_CONNECTIONS, IDR_SPEEDLIMIT };
	m_statusBar.SetPanes(nPanes, countof(nPanes), false);

	m_vertSplitY = CNewzflow::Instance()->settings->GetVertSplitPos();
	m_horzSplitX = CNewzflow::Instance()->settings->GetHorzSplitPos();

	m_TreeView.Init(*this);
	m_NzbView.Init(*this);
	m_RssView.Init(*this);
	m_TvShowView.Init(*this);
	m_MovieView.Init(*this);

	m_TabView[1].Create(*this, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_TOOLTIPS, 0);
	m_ConnectionView.Init(m_TabView[1]);
	m_FileView.Init(m_TabView[1]);
	m_LogView.Init(m_TabView[1]);

	m_TabView[1].AddPage(m_ConnectionView, _T("Connections"));
	m_TabView[1].AddPage(m_LogView, _T("Log"));
	m_TabView[1].AddPage(m_FileView, _T("Files"));
	m_TabView[1].SetActivePage(0);

	// tray notification
	m_TrayIcon.Create(this, IDR_MAINFRAME, _T("Newzflow"), LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME)), Util::MSG_TRAY_NOTIFY, 0, FALSE);

	// get system message font
	NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
	#if(WINVER >= 0x0600)
		OSVERSIONINFO osvi;
		ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&osvi);
		if(osvi.dwMajorVersion < 6) ncm.cbSize -= sizeof(ncm.iPaddedBorderWidth);
	#endif
	if(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, false))
		m_font.CreateFontIndirect(&ncm.lfMessageFont);

	//m_TabView[0].SetFont(m_font);
	m_TabView[1].SetFont(m_font);
	m_LogView.SetFont(m_font);
	
	UpdateLayout();

	SetTimer(1234, 1000, NULL); // GUI update timer; gets distributed to all views

	UIAddToolBar(hWndToolBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);
	UIEnable(ID_NZB_START, FALSE);
	UIEnable(ID_NZB_STOP, FALSE);
	UIEnable(ID_NZB_PAUSE, FALSE);
	UIEnable(ID_NZB_REMOVE, FALSE);
	UIEnable(ID_NZB_MOVE_UP, FALSE);
	UIEnable(ID_NZB_MOVE_DOWN, FALSE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	CComObject<CDropTarget<CMainFrame> >* pDropTarget;
	CComObject<CDropTarget<CMainFrame> >::CreateInstance(&pDropTarget);
	pDropTarget->Register(*this, this);

	Util::SetMainWindow(*this);

	if(CNewzflow::Instance()->ReadQueue()) // TODO: move somewhere else?!
		CNewzflow::Instance()->CreateDownloaders();
	SendMessage(WM_TIMER); // to update views

	m_TreeView.Refresh(); // selects tvDownloads by default

	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	// save window position to settings
	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(&wp);
	CNewzflow::Instance()->settings->SetWindowPos(wp.rcNormalPosition, wp.showCmd);
	CNewzflow::Instance()->settings->SetVertSplitPos(m_vertSplitY);
	CNewzflow::Instance()->settings->SetHorzSplitPos(m_horzSplitX);

	Util::SetMainWindow(NULL);

	::RevokeDragDrop(*this);

	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = FALSE;
	return 1;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnFileAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CString sFilename;
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = *this;
	ofn.lpstrFilter = _T("NZB files (*.nzb)\0*.nzb\0All files (*.*)\0*.*\0\0");
	ofn.lpstrFile = sFilename.GetBuffer(32768);
	ofn.nMaxFile = 32768;
	ofn.Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

	if(GetOpenFileName(&ofn)) {
		TCHAR* path = ofn.lpstrFile;
		TCHAR* fn = path + _tcslen(path) + 1;
		// multi-select
		if(*fn) {
			while(*fn) {
				CNewzflow::Instance()->controlThread->AddFile(CString(path) + CString(_T("\\")) + CString(fn));
				fn += _tcslen(fn) + 1;
			}
		} else {
			CNewzflow::Instance()->controlThread->AddFile(path);
		}
	}

	sFilename.ReleaseBuffer();
	return 0;
}

// modal
class CAddNzbUrlDialog : public CDialogImplEx<CAddNzbUrlDialog>, public CWinDataExchangeEx<CAddNzbUrlDialog>
{
public:
	enum { IDD = IDD_ADD_NZB_URL };

	// Construction
	//CAddNzbUrlDialog();
	//~CAddNzbUrlDialog();

	// Maps
	BEGIN_MSG_MAP(CAddNzbUrlDialog)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER(IDOK, BN_CLICKED, OnOK)
		COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnCancel)
		CHAIN_MSG_MAP(CWinDataExchangeEx<CAddNzbUrlDialog>)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CSettingsServerPage)
		DDX_TEXT(IDC_URL, m_sUrl)
	END_DDX_MAP()

	// Message handlers
	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam)
	{
		CenterWindow(GetParent());
		DoDataExchange(false);
		return TRUE;
	}
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		DoDataExchange(true);
		CString s(m_sUrl);
		s.MakeLower();
		if(s.Find(_T("http://")) != 0 && s.Find(_T("https://")) != 0)
			OnDataCustomError(IDC_URL, _T("Please enter a valid http:// or https:// URL."));
		else
			EndDialog(IDOK);
		return 0;
	}
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(IDCANCEL);
		return 0;
	}

	// DDX variables
	CString m_sUrl;
};

// non-modal
class CSaveNzbDialog : public CDialogImplEx<CSaveNzbDialog>, public CWinDataExchangeEx<CSaveNzbDialog>
{
public:
	enum { IDD = IDD_SAVE_NZB };

	// Construction
	CSaveNzbDialog(CNzb* _nzb, int _errorCode)
	{
		{ NEWZFLOW_LOCK;
		nzb = _nzb;
		nzb->refCount++; // so it doesn't get deleted while we're displaying this dialog
		}
		errorCode = _errorCode;
	}
	~CSaveNzbDialog()
	{
		{ NEWZFLOW_LOCK;
		nzb->refCount--;
		}
	}

	// Maps
	BEGIN_MSG_MAP(CSaveNzbDialog)
		MSG_WM_INITDIALOG(OnInitDialog)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		COMMAND_HANDLER(IDC_DIRECTORY_BUTTON, BN_CLICKED, OnDirectoryButton)
		COMMAND_HANDLER(IDOK, BN_CLICKED, OnOK)
		COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnCancel)
		CHAIN_MSG_MAP(CWinDataExchangeEx<CSaveNzbDialog>)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CSaveNzbDialog)
		DDX_TEXT(IDC_DIRECTORY, m_sDirectory)
	END_DDX_MAP()

	// Message handlers
	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam)
	{
		SetWindowText(nzb->name);
		CenterWindow(GetParent());
		m_sDirectory = CNewzflow::Instance()->settings->GetDownloadDir();
		DoDataExchange(false);
		ShowWindow(SW_SHOW);
		CComboBoxEx wDirectory(GetDlgItem(IDC_DIRECTORY));

		// fill combo box from history array
		CAtlArray<CString>& history = CNewzflow::Instance()->settings->downloadDirHistory;
		for(size_t i = 0; i < history.GetCount(); i++) {
			wDirectory.AddItem(history[i], 0, 0, 0);
		}
		// enable Shell autocomplete
		SHAutoComplete(wDirectory.GetEditCtrl(), SHACF_FILESYS_DIRS);

		if(errorCode != ERROR_SUCCESS) {
			OnDataCustomError(IDC_DIRECTORY, Util::GetErrorMessage(errorCode));
			return FALSE;
		} else {
			return TRUE;
		}
	}
	LRESULT OnDirectoryButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CString sDir;
		GetDlgItemText(IDC_DIRECTORY, sDir);
		sDir = Util::BrowseForFolder(*this, NULL, sDir);
		if(IsWindow()) // window could have been destroyed while folder dialog was blocking us
			SetDlgItemText(IDC_DIRECTORY, sDir);
		return 0;
	}
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		DoDataExchange(true);
		if(nzb->SetPath(m_sDirectory, NULL, &errorCode)) {
			// insert directory at top of history array
			CAtlArray<CString>& history = CNewzflow::Instance()->settings->downloadDirHistory;
			for(size_t i = 0; i < history.GetCount(); i++) {
				if(!m_sDirectory.CompareNoCase(history[i])) {
					history.RemoveAt(i);
					break;
				}
			}
			history.InsertAt(0, m_sDirectory);
			CNewzflow::Instance()->controlThread->CreateDownloaders();
			DestroyWindow();
		} else {
			OnDataCustomError(IDC_DIRECTORY, Util::GetErrorMessage(errorCode));
		}
		return 0;
	}
	void Cancel()
	{
		CNewzflow::Instance()->RemoveNzb(nzb);
		CNewzflow::Instance()->WriteQueue();
		DestroyWindow();
	}
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Cancel();
		return 0;
	}
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		Cancel();
		return 0;
	}
	virtual void OnFinalMessage(HWND)
	{
		delete this;
	}

	// DDX variables
	CString m_sDirectory;
	CNzb* nzb;
	int errorCode;
};

LRESULT CMainFrame::OnFileAddUrl(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAddNzbUrlDialog dlg;
	if(dlg.DoModal(*this)) {
		CNewzflow::Instance()->controlThread->AddURL(dlg.m_sUrl);
	}

	return 0;
}

// this handler is currently used for testing out features
//#include "rss.h"
//#include "HttpDownloader.h"
//	CNewzflow::Instance()->controlThread->AddFile(_T("test\\test.nzb"));
//	CNewzflow::Instance()->controlThread->AddFile(_T("test\\ubuntu-10.04-desktop-i386(devilspeed).par2.nzb"));
//	CNewzflow::Instance()->controlThread->AddFile(_T("test\\VW Sharan-Technik.par2.nzb"));
//	Util::CreateConsole();
//	CNewzflow::Instance()->controlThread->AddURL(_T("http://www.binsearch.info/?action=nzb&45248169=1"));
//	CNewzflow::Instance()->controlThread->AddURL(_T("http://www.nzbindex.com/download/28627887/01-ubuntu-10.10-server-i386.nzb"));
//	CNewzflow::Instance()->controlThread->AddURL(_T("http://nzbmatrix.com/api-nzb-download.php?id=778343&username=XXXXX&apikey=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX&scenename=1"));
//	CRss rss;
//	rss.Parse(_T("test\\rss\\nzbmatrix_download.xml"));
	// this URL requires GZIP handling!
//	CHttpDownloader downloader;
//	downloader.Init();
//	downloader.Download(_T("http://rss.binsearch.net/rss.php?max=50&g=alt.binaries.e-book"), _T("rss"), CString(), NULL);

	// Test UnRAR
/*
	CNzb* nzb = new CNzb;
	for(int i = 1; i <= 42; i++) {
		CNzbFile* file = new CNzbFile(nzb);
		file->fileName.Format(_T("test.part%02d.rar"), i);
		file->subject = file->fileName;
		file->status = kCompleted;
		CNzbSegment* seg = new CNzbSegment(file);
		seg->status = kCompleted;
		file->segments.Add(seg);
		nzb->files.Add(file);
	}
	nzb->refCount = 10;
	CNewzflow::Instance()->nzbs.Add(nzb);
	CNewzflow::Instance()->UpdateFile(nzb->files[0], kCompleted);
*/

LRESULT CMainFrame::OnSaveNzb(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	CNzb* nzb = (CNzb*)wParam;
	int errorCode = (int)lParam;
	CSaveNzbDialog* dlg = new CSaveNzbDialog(nzb, errorCode);
	dlg->Create(*this);

	return 0;
}

class CPowerOffDialog : public CDialogImplEx<CPowerOffDialog>
{
public:
	CPowerOffDialog(Util::EShutdownMode _mode)
	{
		mode = _mode;
	}

	enum { IDD = IDD_POWEROFF };

	BEGIN_MSG_MAP(CPowerOffDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		image = GetDlgItem(IDC_IMAGE);
		image.SetIcon(LoadIcon(LoadLibrary(_T("shell32.dll")), MAKEINTRESOURCE(28)));
		text = GetDlgItem(IDC_TEXT);
		progress = GetDlgItem(IDC_PROGRESS);
		progress.SetRange32(0, 30);
		countdown = 30;
		SetTimer(1234, 1000, NULL);
		BOOL b;
		OnTimer(0, 0, 0, b);
		return TRUE;
	}

	LRESULT OnTimer(WORD /*wNotifyCode*/, WPARAM, LPARAM, BOOL& /*bHandled*/)
	{
		CString s;
		switch(mode) {
		case Util::shutdown_poweroff: s.Format(_T("The computer will power off in %d seconds."), countdown); break;
		case Util::shutdown_suspend: s.Format(_T("The computer will enter standby mode in %d seconds."), countdown); break;
		case Util::shutdown_hibernate: s.Format(_T("The computer will hibernate in %d seconds."), countdown); break;
		}

		text.SetWindowText(CString(_T("All downloads have finished. Auto-Shutdown initiated.\n")) + s);

		if(countdown <= 0)
			EndDialog(IDOK);
		countdown--;
		progress.SetPos(30 - countdown);
		return 0;
	}

	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(IDCANCEL);
		return 0;
	}

public:
	Util::EShutdownMode mode;
	int countdown;
	CStatic text;
	CStatic image;
	CProgressBarCtrl progress;
};

LRESULT CMainFrame::OnNzbFinished(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CString nzbName;
	ENzbStatus nzbStatus;
	{ NEWZFLOW_LOCK;
		CNzb* nzb = (CNzb*)wParam;
		nzbName = nzb->name;
		nzbStatus = nzb->status;
		nzb->refCount--;
	}
	if(m_TrayIcon.IsHidden())
		m_TrayIcon.Show();
	m_TrayIcon.SetBalloonDetails(nzbName, _T("Download Finished"), nzbStatus == kError ? CTrayNotifyIcon::Error : CTrayNotifyIcon::Info, 10000);

	bool allFinished = true;
	{ NEWZFLOW_LOCK;
		CNewzflow* theApp = CNewzflow::Instance();
		for(size_t i = 0; i < theApp->nzbs.GetCount(); i++) {
			if(theApp->nzbs[i]->status != kPaused && theApp->nzbs[i]->status != kFinished && theApp->nzbs[i]->status != kError)
				allFinished = false;
		}
	}

	if(allFinished) {
		Util::EShutdownMode mode = CNewzflow::Instance()->GetShutdownMode();
		if(mode != Util::shutdown_nothing) {
			CNewzflow::Instance()->SetShutdownMode(Util::shutdown_nothing);
			if(mode != Util::shutdown_exit) {
				CPowerOffDialog dlg(mode);
				if(dlg.DoModal(*this) == IDOK)
					Util::Shutdown(mode);
			} else {
				Util::Shutdown(mode);
			}
		}
	}

	return 0;
}

LRESULT CMainFrame::OnTrayNotify(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	if(lParam == NIN_BALLOONTIMEOUT || lParam == NIN_BALLOONUSERCLICK) {
		if(m_TrayIcon.IsShowing())
			m_TrayIcon.Hide();
	}
	m_TrayIcon.OnTrayNotification(wParam, lParam);
	return 0;
}

LRESULT CMainFrame::OnNzbRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	{ NEWZFLOW_LOCK;
		m_LogView.SetNzb(NULL);
		m_FileView.SetNzb(NULL);
		for(int item = m_NzbView.GetNextItem(-1, LVNI_SELECTED); item != -1; item = m_NzbView.GetNextItem(item, LVNI_SELECTED)) {
			CNzb* nzb = (CNzb*)m_NzbView.GetItemData(item);
			CNewzflow::Instance()->RemoveNzb(nzb);
		}
		CNewzflow::Instance()->WriteQueue();
	}
	SendMessage(WM_TIMER);

	return 0;
}

LRESULT CMainFrame::OnNzbMoveUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	{ NEWZFLOW_LOCK;
		for(int item = m_NzbView.GetNextItem(-1, LVNI_SELECTED); item != -1; item = m_NzbView.GetNextItem(item, LVNI_SELECTED)) {
			CNzb* nzb = (CNzb*)m_NzbView.GetItemData(item);
			ASSERT(CNewzflow::Instance()->nzbs[item] == nzb);
			// can't move up if we're already at the top
			if(item == 0)
				break;
			std::swap(CNewzflow::Instance()->nzbs[item-1], CNewzflow::Instance()->nzbs[item]);
		}
	}
	SendMessage(WM_TIMER);
	return 0;
}

LRESULT CMainFrame::OnNzbMoveDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	return 0;
}

LRESULT CMainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndToolBar);
	::ShowWindow(m_hWndToolBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CSettingsSheet::Show(*this);
 
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	CNewzflow* theApp = CNewzflow::Instance();
	theApp->FreeDeletedNzbs();

	// calculate bytes left, current download speed and ETA
	int speed = CNntpSocket::totalSpeed.Get();
	__int64 eta = 0;
	__int64 completed = 0, total = 0, left = 0;
	{
		NEWZFLOW_LOCK;
		size_t count = theApp->nzbs.GetCount();
		for(size_t i = 0; i < count; i++) {
			CNzb* nzb = theApp->nzbs[i];
			for(size_t j = 0; j < nzb->files.GetCount(); j++) {
				CNzbFile* f = nzb->files[j];
				if(f->status == kPaused)
					continue;
				for(size_t k = 0; k < f->segments.GetCount(); k++) {
					CNzbSegment* s = f->segments[k];
					if(s->status == kPaused)
						continue;
					total += s->bytes;
					if(s->status == kCached || s->status == kCompleted || s->status == kError)
						completed += s->bytes;
				}
			}
		}
		if(speed > 1024) {
			eta = (total - completed) / speed;
			left = total - completed;
		}
	}

	// update window title
	CString s = _T("Newzflow r") _T(NEWZFLOW_REVISION);
	if(left > 0)
		s += _T(" - ") + Util::FormatSize(left);
	if(speed > 0)
		s += _T(" - ") + Util::FormatSpeed(speed);
	if(eta > 0)
		s += _T(" - ") + Util::FormatTimeSpan(eta);
	SetWindowText(s);

	// update status bar
	UISetText(1, CNewzflow::Instance()->GetPauseStatus());

	switch(CNewzflow::Instance()->GetShutdownMode()) {
	case Util::shutdown_nothing: s = _T("(Do nothing)"); break;
	case Util::shutdown_exit: s = _T("Exit"); break;
	case Util::shutdown_poweroff: s = _T("Power off"); break;
	case Util::shutdown_suspend: s = _T("Standby mode"); break;
	case Util::shutdown_hibernate: s = _T("Hibernate"); break;
	default: ASSERT(FALSE);
	}
	UISetText(2, CString(_T("On Finish: ")) + s);

	// update taskbar progress list (Windows 7 only)
	if(m_pTaskbarList) {
		if(left > 0) {
			m_pTaskbarList->SetProgressState(*this, TBPF_NORMAL);
			m_pTaskbarList->SetProgressValue(*this, completed, total);
		} else {
			m_pTaskbarList->SetProgressState(*this, TBPF_NOPROGRESS);
		}
	}

	// pass on the message to all child windows to update them
	SendMessageToDescendants(uMsg, wParam, lParam);
	CNewzflow::Instance()->OnTimer();
	UpdateNzbButtons();
	return 0;
}

LRESULT CMainFrame::OnNzbChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	LPNMLISTVIEW lvn = (LPNMLISTVIEW)pnmh;
	if(lvn->hdr.hwndFrom == m_NzbView && (lvn->uChanged & LVIF_STATE) && ((lvn->uOldState & LVIS_SELECTED) != (lvn->uNewState & LVIS_SELECTED))) {
		if(!m_NzbView.IsLockUpdate()) {
			CNzb* nzb = NULL;
			int iSelItem = m_NzbView.GetNextItem(-1, LVNI_SELECTED);
			if(iSelItem >= 0)
				nzb = (CNzb*)m_NzbView.GetItemData(iSelItem);
			m_LogView.SetNzb(nzb);
			m_FileView.SetNzb(nzb);
			UpdateNzbButtons();
		}
	}
	return 0;
}

LRESULT CMainFrame::OnTreeChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	LPNMTREEVIEW lvn = (LPNMTREEVIEW)pnmh;
	if(lvn->hdr.hwndFrom == m_TreeView) {
		CTreeItem ti = m_TreeView.GetSelectedItem();
		if(!ti.IsNull()) {
			CWindow* newTopView = NULL;
			if(ti.GetData() >= CViewTree::kFeeds && ti.GetData() <= CViewTree::kFeedsEnd) {
				newTopView = &m_RssView;
			} else if(ti.GetData() >= CViewTree::kTvShows && ti.GetData() <= CViewTree::kTvShowsEnd) {
				newTopView = &m_TvShowView;
			} else if(ti.GetData() == CViewTree::kMovies) {
				newTopView = &m_MovieView;
			} else {
				newTopView = &m_NzbView;
			}
			if(newTopView != m_pTopView) {
				if(m_pTopView && m_pTopView->IsWindow()) {
					HDWP hdwp = BeginDeferWindowPos(2);
					CRect r;
					m_pTopView->GetWindowRect(&r);
					ScreenToClient(&r);
					m_pTopView->DeferWindowPos(hdwp, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOSIZE | SWP_NOZORDER);
					m_pTopView = newTopView;
					m_pTopView->ModifyStyle(0, WS_BORDER);
					m_pTopView->DeferWindowPos(hdwp, NULL, r.left, r.top, r.Width(), r.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE);
					EndDeferWindowPos(hdwp);
				} else {
					m_pTopView = newTopView;
					m_pTopView->ModifyStyle(0, WS_BORDER);
					m_pTopView->ShowWindow(TRUE);
					UpdateLayout();
				}
			}
			if(m_pTopView == &m_RssView) {
				m_RssView.SetFeed(ti.GetData() - CViewTree::kFeeds);
			} else if(m_pTopView == &m_TvShowView) {
				m_TvShowView.SetShow(ti.GetData() - CViewTree::kTvShows);
			}
		}
	}
	return 0;
}

LRESULT CMainFrame::OnRssFeedUpdated(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_TreeView.Refresh();
	return 0;
}

LRESULT CMainFrame::OnMoviesUpdated(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_MovieView.Refresh();
	return 0;
}

void CMainFrame::UpdateNzbButtons()
{
	bool enable = m_NzbView.GetSelectedCount() > 0;
	/*
	bool allStopped = true;
	bool allStarted = true;
	int item = -1;
	for(;;) {
	item = m_list.GetNextItem(item, LVNI_SELECTED);
	if(item == -1)
	break;
	CNzb* nzb = (CNzb*)m_list.GetItemData(item);
	if(nzb->status != kDownloading) allStarted = FALSE;
	}
	*/
	UIEnable(ID_NZB_START, enable);
	UIEnable(ID_NZB_PAUSE, enable);
	UIEnable(ID_NZB_STOP, enable);
	UIEnable(ID_NZB_REMOVE, enable);
	UIEnable(ID_NZB_MOVE_UP, enable);
	UIEnable(ID_NZB_MOVE_DOWN, enable);
}

LRESULT CMainFrame::OnTaskbarButtonCreated(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DWORD dwMajor = LOBYTE(LOWORD(GetVersion()));
	DWORD dwMinor = HIBYTE(LOWORD(GetVersion()));

	// Check that the OS is Win 7 or later (Win 7 is v6.1).
	if(dwMajor > 6 || (dwMajor == 6 && dwMinor > 0)) {	
		m_pTaskbarList.Release();
		m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);
	}

	return 0;
}

// another Newzflow instance notifies us of a new NZB that should be added
LRESULT CMainFrame::OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	COPYDATASTRUCT* cds = (COPYDATASTRUCT*)lParam;

	CString str((LPCTSTR)cds->lpData, cds->cbData / sizeof(TCHAR));
	CNewzflow::Instance()->controlThread->AddFile(str);

	return 0;
}

void CMainFrame::OnDropFile(LPCTSTR szBuff)
{
	CNewzflow::Instance()->controlThread->AddFile(szBuff);
}

void CMainFrame::OnDropURL(LPCTSTR szBuff)
{
	CNewzflow::Instance()->controlThread->AddURL(szBuff);
}

// CNewzflowStatusBarCtrl
//////////////////////////////////////////////////////////////////////////

LRESULT CNewzflowStatusBarCtrl::OnRButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
	// right click on speed limit pane brings up context menu to change speed limit
	lParam = GetMessagePos();
	CPoint ptScreen(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), pt(ptScreen);
	ScreenToClient(&pt);
	CRect rPane;
	GetPaneRect(IDR_PAUSE, rPane);
	if(rPane.PtInRect(pt)) {
		OnRClickPause(ptScreen);
		return 0;
	}
	GetPaneRect(IDR_SHUTDOWN, rPane);
	if(rPane.PtInRect(pt)) {
		OnRClickShutdown(ptScreen);
		return 0;
	}
	GetPaneRect(IDR_CONNECTIONS, rPane);
	if(rPane.PtInRect(pt)) {
		OnRClickConnections(ptScreen);
		return 0;
	}
	this->GetPaneRect(IDR_SPEEDLIMIT, rPane);
	if(rPane.PtInRect(pt)) {
		OnRClickSpeedLimit(ptScreen);
		return 0;
	}
	return 0;
}

void CNewzflowStatusBarCtrl::OnRClickSpeedLimit(const CPoint& ptScreen)
{
	int curLimit = CNntpSocket::speedLimiter.GetLimit();
	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenu((curLimit == 0 ? MF_CHECKED : 0) | MF_STRING, (UINT_PTR)100000, _T("Unlimited"));
	menu.AppendMenu(MF_SEPARATOR);
	CAtlArray<int> limits;
	if(curLimit == 0) {
		for(int i = 10; i <= 50; i += 10) limits.Add(i*1024);
		for(int i = 100; i <= 500; i += 100) limits.Add(i*1024);
		for(int i = 1; i < 10; i += 2) limits.Add(i*1024*1000);
	} else {
		int l = curLimit / 1024;
		int mul = 1;
		if(l >= 300) mul = 5;
		if(l >= 500) mul = 10;
		if(l >= 1000) mul = 20;
		if(l >= 3000) mul = 50;
		if(l <= 60000) limits.Add(l*1024);
		if(l+mul*1 <= 60000) limits.Add((l+mul*1)*1024);
		if(l+mul*2 <= 60000) limits.Add((l+mul*2)*1024);
		if(l-1 >= 1) limits.InsertAt(0, (l-mul*1)*1024);
		if(l-2 >= 1) limits.InsertAt(0, (l-mul*2)*1024);
		int l5 = (l-2) - (l-2)%(mul*5);
		if(l5 >= 1 && l5 % (mul*10)) limits.InsertAt(0, l5*1024);
		int l10 = l5 - l5%(mul*10);
		if(l10 >= 1) limits.InsertAt(0, l10*1024);
		if(l10-mul*10 >= 1) limits.InsertAt(0, (l10-10)*1024);
		if(l10-mul*20 >= 1) limits.InsertAt(0, (l10-mul*20)*1024);
		if(l10-mul*30 >= 1) limits.InsertAt(0, (l10-mul*30)*1024);
		if(l10-mul*40 >= 1) limits.InsertAt(0, (l10-mul*40)*1024);
		l5 = (l+mul*2) - (l+mul*2)%(mul*5) + mul*5;
		if(l5 <= 60000) limits.Add(l5*1024);
		l10 = l5 - l5%(mul*10) + mul*10;
		if(l10 <= 60000) limits.Add(l10*1024);
		if(l10+mul*10 <= 60000) limits.Add((l10+mul*10)*1024);
		if(l10+mul*20 <= 60000) limits.Add((l10+mul*20)*1024);
		int l50 = (l10+mul*20) - (l10+mul*20)%(mul*50) + mul*50;
		if(l50 <= 60000) limits.Add(l50*1024);
		if(l50+mul*50 <= 60000) limits.Add((l50+mul*50)*1024);
		if(l50+mul*150 <= 60000) limits.Add((l50+mul*150)*1024);
	}
	for(size_t i = 0; i < limits.GetCount(); i++) {
		CString s;
		s.Format(_T("%d kB/s"), limits[i] / 1024);
		menu.AppendMenu((curLimit == limits[i]  ? MF_CHECKED : 0) | MF_STRING, limits[i], s);
	}
	int ret = (int)menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RETURNCMD, ptScreen.x, ptScreen.y, *this);
	if(ret != 0) {
		int newLimit = (ret == 100000) ? 0 : ret;
		CNewzflow::Instance()->SetSpeedLimit(newLimit);
	}
}

void CNewzflowStatusBarCtrl::OnRClickConnections(const CPoint &ptScreen)
{
	int curLimit = CNewzflow::Instance()->settings->GetConnections();
	CMenu menu;
	menu.CreatePopupMenu();
	CAtlArray<int> limits;
	int l = curLimit;
	if(l <= 100) limits.Add(l);
	if(l+1 <= 100) limits.Add((l+1));
	if(l+2 <= 100) limits.Add((l+2));
	if(l-1 >= 1) limits.InsertAt(0, (l-1));
	if(l-2 >= 1) limits.InsertAt(0, (l-2));
	int l5 = (l-2) - (l-2)%(5);
	if(l5 >= 1 && l5 % (10)) limits.InsertAt(0, l5);
	int l10 = l5 - l5%(10);
	if(l10 >= 1) limits.InsertAt(0, l10);
	if(l10-10 >= 1) limits.InsertAt(0, (l10-10));
	if(l10-20 >= 1) limits.InsertAt(0, (l10-20));
	if(l10-30 >= 1) limits.InsertAt(0, (l10-30));
	if(l10-40 >= 1) limits.InsertAt(0, (l10-40));
	l5 = (l+2) - (l+2)%(5) + 5;
	if(l5 <= 100) limits.Add(l5);
	l10 = l5 - l5%(10) + 10;
	if(l10 <= 100) limits.Add(l10);
	if(l10+10 <= 100) limits.Add((l10+10));
	if(l10+20 <= 100) limits.Add((l10+20));
	int l50 = (l10+20) - (l10+20)%(50) + 50;
	if(l50 <= 100) limits.Add(l50);
	if(l50+50 <= 100) limits.Add((l50+50));
	if(l50+150 <= 100) limits.Add((l50+150));

	for(size_t i = 0; i < limits.GetCount(); i++) {
		CString s;
		s.Format(_T("%d"), limits[i]);
		menu.AppendMenu((curLimit == limits[i]  ? MF_CHECKED : 0) | MF_STRING, limits[i], s);
	}
	int ret = (int)menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RETURNCMD, ptScreen.x, ptScreen.y, *this);
	if(ret != 0) {
		CNewzflow::Instance()->settings->SetConnections(ret);
		CNewzflow::Instance()->OnServerSettingsChanged();
	}
}

void CNewzflowStatusBarCtrl::OnRClickPause(const CPoint &ptScreen)
{
	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenu((!CNewzflow::Instance()->IsPaused() ? MF_CHECKED : 0) | MF_STRING, 1, _T("Resume"));
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu((CNewzflow::Instance()->IsPaused() ? MF_CHECKED : 0) | MF_STRING, 2, _T("Pause"));
	menu.AppendMenu(MF_STRING, 5*60, _T("Pause for 5 minutes"));
	menu.AppendMenu(MF_STRING, 10*60, _T("Pause for 10 minutes"));
	menu.AppendMenu(MF_STRING, 15*60, _T("Pause for 15 minutes"));
	menu.AppendMenu(MF_STRING, 30*60, _T("Pause for 30 minutes"));
	menu.AppendMenu(MF_STRING, 1*60*60, _T("Pause for 1 hour"));
	menu.AppendMenu(MF_STRING, 3*60*60, _T("Pause for 3 hours"));
	menu.AppendMenu(MF_STRING, 6*60*60, _T("Pause for 6 hours"));
	int ret = (int)menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RETURNCMD, ptScreen.x, ptScreen.y, *this);
	if(ret != 0) {
		CNewzflow::Instance()->Pause(ret > 1, ret > 2 ? ret : INT_MAX);
		Util::GetMainWindow().SendMessage(WM_TIMER);
	}
}

void CNewzflowStatusBarCtrl::OnRClickShutdown(const CPoint &ptScreen)
{
	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenu(MF_STRING, Util::shutdown_nothing + 1, _T("Do nothing"));
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, Util::shutdown_exit + 1, _T("Exit Newzflow"));
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, Util::shutdown_poweroff + 1, _T("Power off Computer"));
	menu.AppendMenu(MF_STRING, Util::shutdown_suspend + 1, _T("Standby Computer"));
	menu.AppendMenu(MF_STRING, Util::shutdown_hibernate + 1, _T("Hibernate Computer"));
	int ret = (int)menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RETURNCMD, ptScreen.x, ptScreen.y, *this);
	if(ret != 0) {
		CNewzflow::Instance()->SetShutdownMode((Util::EShutdownMode)(ret-1));
		Util::GetMainWindow().SendMessage(WM_TIMER);
	}
}
