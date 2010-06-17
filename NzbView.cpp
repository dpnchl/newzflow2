// NzbView.cpp : implementation of the CNzbView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "NzbView.h"
#include "Newzflow.h"
#include "Util.h"

BOOL CNzbView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

// columns
enum {
	kOrder,
	kName,
	kSize,
	kDone,
	kStatus,
};

void CNzbView::Init(HWND hwndParent)
{
	Create(hwndParent, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS, 0);

	m_thmProgress.OpenThemeData(*this, L"PROGRESS");

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(*this, L"explorer", NULL);
	AddColumn(_T("#"), kOrder, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	AddColumn(_T("Name"), kName);
	AddColumn(_T("Size"), kSize, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	AddColumn(_T("Done"), kDone, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	AddColumn(_T("Status"), kStatus);
	SetColumnWidth(kSize, 80);
	SetColumnWidth(kName, 400);
	SetColumnWidth(kDone, 80);
	SetColumnWidth(kStatus, 120);
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
				AddItem(i, kOrder, s);
			}
			SetItemData(i, (DWORD_PTR)nzb);
			AddItem(i, kName, nzb->name);
			__int64 completed = 0, total = 0;
			for(size_t j = 0; j < nzb->files.GetCount(); j++) {
				CNzbFile* f = nzb->files[j];
				for(size_t k = 0; k < f->segments.GetCount(); k++) {
					CNzbSegment* s = f->segments[k];
					total += s->bytes;
					if(s->status == kCompleted)
						completed += s->bytes;
				}
			}
			AddItem(i, kSize, Util::FormatSize(total));
			s.Format(_T("%.1f%%"), 100. * (double)completed / (double)total);
			AddItem(i, kDone, s);
			AddItem(i, kStatus, GetNzbStatusString(nzb->status));
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

DWORD CNzbView::OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
{
	return CDRF_NOTIFYITEMDRAW;
}

DWORD CNzbView::OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
{
	return CDRF_NOTIFYSUBITEMDRAW;
}

DWORD CNzbView::OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW lpNMCustomDraw)
{
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)lpNMCustomDraw;
	if(cd->iSubItem == kDone) {
		CDCHandle dcPaint(cd->nmcd.hdc);

		// draw progress frame
		CRect rcProgress(cd->nmcd.rc);
		rcProgress.DeflateRect(3, 2);
		m_thmProgress.DrawThemeBackground(dcPaint, PP_BAR, 0, rcProgress, NULL);

		CString strItemText;
		GetItemText(cd->nmcd.dwItemSpec, cd->iSubItem, strItemText);

		// draw progress bar															
		//rcProgress.DeflateRect(1, 1, 1, 1);
		rcProgress.right = rcProgress.left + (int)( (double)rcProgress.Width() * ((max(min(_tstof(strItemText), 100), 0)) / 100.0));
		m_thmProgress.DrawThemeBackground(dcPaint, PP_FILL, 0, rcProgress, NULL);

		m_thmProgress.DrawThemeText(dcPaint, cd->iPartId, cd->iStateId, strItemText, -1, DT_CENTER | DT_SINGLELINE | DT_VCENTER, 0, &cd->nmcd.rc);

		return CDRF_SKIPDEFAULT;
	}
	return CDRF_DODEFAULT;
}
