// ConnectionView.cpp : implementation of the CConnectionView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "Newzflow.h"
#include "Downloader.h"
#include "ConnectionView.h"
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
	AddColumn(_T("#"), 0, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
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
		size_t count = CNewzflow::Instance()->downloaders.GetCount();
		for(size_t i = 0; i < count; i++) {
			CDownloader* dl = CNewzflow::Instance()->downloaders[i];
			CString s;
			s.Format(_T("%d"), i+1);
			if((int)i >= lvCount) {
				AddItem(i, 0, s);
			}
			int speed = dl->sock.speed.Get();
			AddItem(i, 1, (speed > 0) ? Util::FormatSpeed(speed) : _T(""));
			CString sCommand, sStatus;
			AddItem(i, 2, dl->sock.GetLastCommand());
			AddItem(i, 3, dl->sock.GetLastReply());
		}
		SetRedraw(FALSE);
		while((int)count < lvCount) {
			DeleteItem(count);
			lvCount--;
		}
		SetRedraw(TRUE);
	}

	return 0;
}