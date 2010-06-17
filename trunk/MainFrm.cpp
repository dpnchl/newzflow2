// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "MainFrm.h"

//////////////////////////////////////////////////////////////////////////

// BARTO START
#include "util.h"
#include "nzb.h"
#include "sock.h"
#include "Downloader.h"
#include "DiskWriter.h"
#include "Newzflow.h"

class CBartoThread : public CThreadImpl<CBartoThread>
{
	DWORD Run() {
		CNewzflow* theApp = CNewzflow::Instance();
		VERIFY(CNntpSocket::InitWinsock());

		Util::print("Control Thread starting");

		CDiskWriter diskWriter;
		//CNzb* nzb = CNzb::Create(_T("file://c:/Users/Barto/Documents/Visual Studio 2008/Projects/Newzflow/test/VW Sharan-Technik.par2.corrupt.nzb"));
		CNzb* nzb = CNzb::Create(_T("file://c:/Users/Barto/Documents/Visual Studio 2008/Projects/Newzflow/test/ubuntu-10.04-desktop-i386(devilspeed).par2.corrupt.nzb"));
		//CNzb* nzb2 = CNzb::Create(_T("file://c:/Users/Barto/Documents/Visual Studio 2008/Projects/Newzflow/test/VW Kever 1200-1200A-1300-1300A-1500 tot 1967 Manual GE.par2.nzb"));
		ASSERT(nzb);
		{ CNewzflow::CLock lock;
			theApp->nzbs.Add(nzb);
			//theApp->nzbs.Add(nzb2);

			for(int i = 0; i < 5; i++) {
				CNewzflow::Instance()->downloaders.Add(new CDownloader);
			}
		}

		Util::print("waiting for downloaders to finish...\n");
		bool active = true;
		while(active) {
			CString s;
			s.Format(_T("Current Speed: %.2fkb/s"), CNntpSocket::totalSpeed.Get() / 1024.f);
			Util::print(CStringA(s));

			{ CNewzflow::CLock lock;
				bool allFinished = true;
				for(size_t i = 0; i < CNewzflow::Instance()->downloaders.GetCount(); i++) {
					if(CNewzflow::Instance()->downloaders[i]->GetExitCode() == STILL_ACTIVE)
						allFinished = false;
				}
				if(allFinished) active = false;
			}
			Sleep(2000);
		}

		CNntpSocket::CloseWinsock();

		Util::print("waiting for disk writer to finish...\n");
		diskWriter.PostQuitMessage();
		diskWriter.Join();

		{ CNewzflow::CLock lock;
			bool allFinished = true;
			for(size_t i = 0; i < CNewzflow::Instance()->downloaders.GetCount(); i++) {
				delete CNewzflow::Instance()->downloaders[i];
			}
			CNewzflow::Instance()->downloaders.RemoveAll();
		}

		Util::print("Control Thread shutting down");
		return 0;
	}
};

//////////////////////////////////////////////////////////////////////////


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

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CreateSimpleToolBar();

	CreateSimpleStatusBar();

	m_hWndClient = m_split.Create(*this, rcDefault, NULL, 0, WS_EX_CLIENTEDGE);

	m_list.Init(m_split);

	m_tab.Create(m_split, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_TOOLTIPS, 0);

	m_connections.Init(m_tab);
	m_files.Init(m_tab);

	m_tab.AddPage(m_connections, _T("Connections"));
	m_tab.AddPage(m_files, _T("Files"));
	m_tab.SetActivePage(0);

	m_split.SetSplitterExtendedStyle(SPLIT_BOTTOMALIGNED, 0);
	m_split.SetSplitterPanes(m_list, m_tab, false);

	NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };

	if(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, false))
		m_font.CreateFontIndirect(&ncm.lfMessageFont);

	m_tab.SetFont(m_font);
	
	/*
	m_connections.SetFont(m_font);
	m_split.SetFont(m_font);
	m_list.SetFont(m_font);
*/
	UpdateLayout();
	CRect r;
	m_split.GetClientRect(&r);
	m_split.SetSplitterPos(r.Height() - 300);

	SetTimer(1234, 1000, NULL); // GUI update timer; gets distributed to all views

	UIAddToolBar(m_hWndToolBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
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

LRESULT CMainFrame::OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CBartoThread* barto = new CBartoThread;

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
	SendMessageToDescendants(uMsg, wParam, lParam);
	return 0;
}

LRESULT CMainFrame::OnNzbChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	LPNMLISTVIEW lvn = (LPNMLISTVIEW)pnmh;
	if(lvn->hdr.hwndFrom == m_list && (lvn->uChanged & LVIF_STATE) && ((lvn->uOldState & LVIS_SELECTED) != (lvn->uNewState & LVIS_SELECTED))) {
		m_files.SetNzb((CNzb*)m_list.GetItemData(m_list.GetNextItem(-1, LVNI_SELECTED)));
	}
	return 0;
}