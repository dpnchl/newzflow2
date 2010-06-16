// NzbView.cpp : implementation of the CNzbView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "NzbView.h"
#include "Newzflow.h"

BOOL CNzbView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

void CNzbView::Init(HWND hwndParent)
{
	Create(hwndParent, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS, 0);

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	AddColumn(_T("#"), 0, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	AddColumn(_T("Name"), 1);
	AddColumn(_T("Progress"), 2, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	SetColumnWidth(1, 400);
	SetColumnWidth(2, 100);
}

LRESULT CNzbView::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	int lvCount = GetItemCount();

	{ CNewzflow::CLock lock;
		CNewzflow* theApp = CNewzflow::Instance();
		size_t count = theApp->nzbs.GetCount();
		for(size_t i = 0; i < count; i++) {
			CNzb* nzb = theApp->nzbs[i];
			CString s;
			s.Format(_T("%d"), i+1);
			if(i >= lvCount) {
				AddItem(i, 0, s);
			}
			SetItemData(i, (DWORD_PTR)nzb);
			AddItem(i, 1, nzb->name);
			int completed = 0;
			for(size_t j = 0; j < nzb->files.GetCount(); j++) {
				if(nzb->files[j]->status == kCompleted)
					completed++;
			}
			s.Format(_T("%d/%d"), completed, nzb->files.GetCount());
			AddItem(i, 2, s);
		}
		SetRedraw(FALSE);
		while(count < lvCount) {
			DeleteItem(count);
			lvCount--;
		}
		SetRedraw(TRUE);
	}

	return 0;
}
