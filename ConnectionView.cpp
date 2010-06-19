// ConnectionView.cpp : implementation of the CConnectionView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "Newzflow.h"
#include "Downloader.h"
#include "ConnectionView.h"

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
	AddColumn(_T("Blabla"), 2);
	SetColumnWidth(1, 80);
	SetColumnWidth(2, 500);
}

LRESULT CConnectionView::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	int lvCount = GetItemCount();

	{ CNewzflow::CLock lock;
		size_t count = CNewzflow::Instance()->downloaders.GetCount();
		for(size_t i = 0; i < count; i++) {
			CDownloader* dl = CNewzflow::Instance()->downloaders[i];
			CString s;
			s.Format(_T("%d"), i+1);
			if((int)i >= lvCount) {
				AddItem(i, 0, s);
			}
			s.Format(_T("%.1f kB/s"), dl->sock.speed.Get() / 1024.f);
			AddItem(i, 1, s);
			{ CComCritSecLock<CComAutoCriticalSection> lock(dl->sock.cs);
				s = dl->sock.lastCommand;
			}

			AddItem(i, 2, s);
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