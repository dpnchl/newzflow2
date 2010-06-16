// FileView.cpp : implementation of the CFileView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "FileView.h"
#include "Newzflow.h"

CFileView::CFileView()
{
	nzb = NULL;
}

BOOL CFileView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

void CFileView::Init(HWND hwndParent)
{
	Create(hwndParent, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS, 0);

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	AddColumn(_T("Name"), 0);
	AddColumn(_T("Size"), 1, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	AddColumn(_T("Done"), 2, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	AddColumn(_T("%"), 3, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	AddColumn(_T("# Segments"), 4, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	SetColumnWidth(0, 400);
	SetColumnWidth(1, 80);
	SetColumnWidth(2, 80);
	SetColumnWidth(3, 80);
}

LRESULT CFileView::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if(!nzb) {
		if(GetItemCount() > 0)
			DeleteAllItems();
		return 0;
	}

	int lvCount = GetItemCount();

	{ CNewzflow::CLock lock;
		CNewzflow* theApp = CNewzflow::Instance();
		size_t count = nzb->files.GetCount();
		for(size_t i = 0; i < count; i++) {
			CNzbFile* file = nzb->files[i];
			CString s = file->subject;
			if(i >= lvCount) {
				AddItem(i, 0, s);
			} else {
				SetItemText(i, 0, s);
			}
			__int64 size = 0, done = 0;
			for(size_t j = 0; j < file->segments.GetCount(); j++) {
				size += file->segments[j]->bytes;
				if(file->segments[j]->status == kCompleted)
					done += file->segments[j]->bytes;
			}
			s.Format(_T("%.1f kB"), (float)size / 1024.f);
			AddItem(i, 1, s);
			s.Format(_T("%.1f kB"), (float)done / 1024.f);
			AddItem(i, 2, s);
			s.Format(_T("%.1f %%"), 100.f * (float)done / (float)size);
			AddItem(i, 3, s);
			s.Format(_T("%d"), file->segments.GetCount());
			AddItem(i, 4, s);
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

void CFileView::SetNzb(CNzb* _nzb)
{
	nzb = _nzb;
	BOOL b;
	OnTimer(0, 0, 0, b);
}