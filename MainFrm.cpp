// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "MainFrm.h"

#include "Newzflow.h"
#include "Settings.h"
#include "Sock.h"
#include "Util.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

const UINT CMainFrame::s_msgTaskbarButtonCreated = RegisterWindowMessage(_T("TaskbarButtonCreated"));

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;

	return m_list.PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle()
{
	UIUpdateToolBar();
	return FALSE;
}

void CMainFrame::UpdateLayout(BOOL bResizeBars)
{
	CRect rect;
	GetClientRect(rect);

	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	CRect r1(rect), r2(rect);
	m_vertSplitYReal = max(100, rect.bottom - max(100, m_vertSplitY));
	r1.bottom = m_vertSplitYReal - 3;
	r2.top = m_vertSplitYReal + 3;

	m_list.SetWindowPos(NULL, r1, SWP_NOZORDER | SWP_NOACTIVATE);
	m_tab.SetWindowPos(NULL, r2, SWP_NOZORDER | SWP_NOACTIVATE);
}

LRESULT CMainFrame::OnSetCursor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	const DWORD pos = GetMessagePos();
	CPoint pt(GET_X_LPARAM(pos), GET_Y_LPARAM(pos));
	ScreenToClient(&pt);

	if(pt.y >= m_vertSplitYReal - 3 && pt.y <= m_vertSplitYReal + 3) {
		SetCursor(LoadCursor(NULL, IDC_SIZENS));
		return 1;
	}

	bHandled = FALSE;
	return 0;
}

LRESULT CMainFrame::OnMouseMove(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if( (wParam & MK_LBUTTON) != 0 && ::GetCapture() == *this) {
		lParam = GetMessagePos();
		CPoint ptMouseCur(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		CRect r; GetClientRect(r);
		m_vertSplitY = max(100, min(r.Height() - 100, m_ptSplit.y - (ptMouseCur.y - m_ptMouse.y)));
		UpdateLayout();
		SetCursor(LoadCursor(NULL, IDC_SIZENS));
		return 1;
	}
	bHandled = FALSE;
	return 0;
}

LRESULT CMainFrame::OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
	lParam = GetMessagePos();
	CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	ScreenToClient(&pt);
	if(pt.y >= m_vertSplitYReal - 3 && pt.y <= m_vertSplitYReal + 3) {
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

	if(!m_toolBarImageList.Load(CNewzflow::Instance()->settings->GetAppDataDir() + _T("toolbar.bmp"), 24, 24))
		m_toolBarImageList.LoadFromResource(IDB_TOOLBAR, 24, 24);
	m_toolBarImageList.Set(hWndToolBar);

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndToolBar);

	CreateSimpleStatusBar();

	m_vertSplitY = CNewzflow::Instance()->settings->GetSplitPos();

	m_list.Init(*this);

	m_tab.Create(*this, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_TOOLTIPS, 0);

	m_connections.Init(m_tab);
	m_files.Init(m_tab);

	m_tab.AddPage(m_connections, _T("Connections"));
	m_tab.AddPage(m_files, _T("Files"));
	m_tab.SetActivePage(0);

	NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };

	if(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, false))
		m_font.CreateFontIndirect(&ncm.lfMessageFont);

	m_tab.SetFont(m_font);
	
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

	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	// save window position to settings
	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(&wp);
	CNewzflow::Instance()->settings->SetWindowPos(wp.rcNormalPosition, wp.showCmd);
	CNewzflow::Instance()->settings->SetSplitPos(m_vertSplitY);

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
//	CNewzflow::Instance()->controlThread->AddFile(_T("test\\test.nzb"));
	CNewzflow::Instance()->controlThread->AddFile(_T("test\\ubuntu-10.04-desktop-i386(devilspeed).par2.nzb"));

	return 0;
}

LRESULT CMainFrame::OnNzbRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	{ CNewzflow::CLock lock;
		for(int item = m_list.GetNextItem(-1, LVNI_SELECTED); item != -1; item = m_list.GetNextItem(item, LVNI_SELECTED)) {
			CNzb* nzb = (CNzb*)m_list.GetItemData(item);
			CNewzflow* theApp = CNewzflow::Instance();
			for(size_t i = 0; i < theApp->nzbs.GetCount(); i++) {
				if(theApp->nzbs[i] == nzb) {
					// if NZB's refCount is 0, we can delete it immediately
					// otherwise there are still downloaders or the post processor active,
					// so just move it to a different array where we will periodically check whether we can delete it for good
					if(nzb->refCount == 0)
						delete nzb;
					else {
						TRACE(_T("Can't remove %s yet. refCount=%d\n"), nzb->name, nzb->refCount);
						theApp->deletedNzbs.Add(nzb);
					}
					theApp->nzbs.RemoveAt(i);
					break;
				}
			}
		}
	}
	SendMessage(WM_TIMER);

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

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	CNewzflow* theApp = CNewzflow::Instance();
	// check if we can remove some of the deleted NZBs
	{ CNewzflow::CLock lock;
		for(size_t i = 0; i < theApp->deletedNzbs.GetCount(); i++) {
			if(theApp->deletedNzbs[i]->refCount == 0) {
				TRACE(_T("Finally can remove %s.\n"), theApp->deletedNzbs[i]->name);
				delete theApp->deletedNzbs[i];
				theApp->deletedNzbs.RemoveAt(i);
				i--;
			}
		}
	}

	// calculate bytes left, current download speed and ETA
	int speed = CNntpSocket::totalSpeed.Get();
	__int64 eta = 0;
	__int64 completed = 0, total = 0, left = 0;
	if(speed > 1024) {
		CNewzflow::CLock lock;
		size_t count = theApp->nzbs.GetCount();
		for(size_t i = 0; i < count; i++) {
			CNzb* nzb = theApp->nzbs[i];
			for(size_t j = 0; j < nzb->files.GetCount(); j++) {
				CNzbFile* f = nzb->files[j];
				for(size_t k = 0; k < f->segments.GetCount(); k++) {
					CNzbSegment* s = f->segments[k];
					total += s->bytes;
					if(s->status == kCompleted || s->status == kError)
						completed += s->bytes;
				}
			}
		}
		eta = (total - completed) / speed;
		left = total - completed;
	}

	CString s = _T("Newzflow");
	if(left > 0)
		s += _T(" - ") + Util::FormatSize(left);
	if(speed > 0)
		s += _T(" - ") + Util::FormatSpeed(speed);
	if(eta > 0)
		s += _T(" - ") + Util::FormatTimeSpan(eta);
	SetWindowText(s);

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
	return 0;
}

LRESULT CMainFrame::OnNzbChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	LPNMLISTVIEW lvn = (LPNMLISTVIEW)pnmh;
	if(lvn->hdr.hwndFrom == m_list && (lvn->uChanged & LVIF_STATE) && ((lvn->uOldState & LVIS_SELECTED) != (lvn->uNewState & LVIS_SELECTED))) {
		m_files.SetNzb((CNzb*)m_list.GetItemData(m_list.GetNextItem(-1, LVNI_SELECTED)));
		bool enable = m_list.GetSelectedCount() > 0;
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
	return 0;
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