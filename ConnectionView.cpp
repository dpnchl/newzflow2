// ConnectionView.cpp : implementation of the CConnectionView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "Newzflow.h"
#include "Downloader.h"
#include "ConnectionView.h"
#include "RssWatcher.h"
#include "Util.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

BOOL CConnectionView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}


void CConnectionView::Init(HWND hwndParent)
{
	Create(hwndParent, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT, 0);

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(*this, L"explorer", NULL);
	AddColumn(_T("Task"), 0, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	AddColumn(_T("Speed"), 1, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	AddColumn(_T("Command"), 2);
	AddColumn(_T("Status"), 3);
	SetColumnWidth(1, 80);
	SetColumnWidth(2, 400);
	SetColumnWidth(3, 250);
}

LRESULT CConnectionView::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	int lvCount = GetItemCount();

	{ NEWZFLOW_LOCK;
		// downloaders
		size_t count = CNewzflow::Instance()->downloaders.GetCount();
		size_t i;
		for(i = 0; i < count; i++) {
			CDownloader* dl = CNewzflow::Instance()->downloaders[i];
			CString s;
			s.Format(_T("DL%d"), i+1);
			if((int)i >= lvCount)
				AddItem(i, 0, s);
			else
				SetItemText(i, 0, s);
			int speed = dl->sock.speed.Get();
			AddItem(i, 1, (speed > 0) ? Util::FormatSpeed(speed) : _T(""));
			AddItem(i, 2, dl->sock.GetLastCommand());
			AddItem(i, 3, dl->sock.GetLastReply());
		}
		// RSS watcher
		{
			CString rssCommand = CNewzflow::Instance()->rssWatcher->GetLastCommand();
			CString rssReply = CNewzflow::Instance()->rssWatcher->GetLastReply();

			if(!rssCommand.IsEmpty() || !rssReply.IsEmpty()) {
				if((int)i >= lvCount)
					AddItem(i, 0, _T("RSS"));
				else
					SetItemText(i, 0, _T("RSS"));
				SetItemText(i, 1, _T(""));
				SetItemText(i, 2, rssCommand);
				SetItemText(i, 3, rssReply);
				i++;
			}
		}

		SetRedraw(FALSE);
		while((int)i < lvCount) {
			DeleteItem(count);
			lvCount--;
		}
		SetRedraw(TRUE);
	}

	return 0;
}