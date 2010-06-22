// NzbView.cpp : implementation of the CNzbView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "NzbView.h"
#include "Newzflow.h"
#include "Util.h"
#include "Settings.h"
#include "sock.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

CNzbView::~CNzbView()
{
	m_imageList.Destroy();
}

BOOL CNzbView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

// image list - µTorrent compatible
// ilQueued comes from own resource, it's not in µTorrent's image list, so all µTorrent's IDs are offset by 1
enum {
	ilQueued = 0,
	ilDownloading = 1,
	ilStopped = 3,
	ilPaused = 4,
	ilError = 7,
	ilCompleted = 8,
	ilVerifying = 12,
};

// columns
enum {
	kName,
	kOrder,
	kSize,
	kDone,
	kStatus,
	kETA,
};

void CNzbView::Init(HWND hwndParent)
{
	Create(hwndParent, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS, 0);

	m_thmProgress.OpenThemeData(*this, L"PROGRESS");

	m_imageList.Create(16, 16, ILC_COLOR32, 0, 100);
	CImage image;
	image.LoadFromResource(ModuleHelper::GetResourceInstance(), IDB_STATUS_OWN); // first add "own" image
	m_imageList.Add(image, (HBITMAP)NULL);
	image.Destroy();
	if(!SUCCEEDED(image.Load(CNewzflow::Instance()->settings->GetAppDataDir() + _T("tstatus.bmp")))) // then try to load µTorrent-compatible images from disk... 
		image.LoadFromResource(ModuleHelper::GetResourceInstance(), IDB_STATUS); // ...or from resource
	m_imageList.Add(image, (HBITMAP)NULL);
	SetImageList(m_imageList, LVSIL_SMALL);

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP);
	SetWindowTheme(*this, L"explorer", NULL);
	AddColumn(_T("#"), kOrder, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	AddColumn(_T("Name"), kName);
	AddColumn(_T("Size"), kSize, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	AddColumn(_T("Done"), kDone, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	AddColumn(_T("Status"), kStatus);
	AddColumn(_T("ETA"), kETA);
	SetColumnWidth(kSize, 80);
	SetColumnWidth(kName, 400);
	SetColumnWidth(kDone, 80);
	SetColumnWidth(kStatus, 120);
	SetColumnWidth(kETA, 80);
	CNewzflow::Instance()->settings->GetListViewColumns(_T("NzbView"), *this);
}

LRESULT CNzbView::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	int lvCount = GetItemCount();

	int speed = CNntpSocket::totalSpeed.Get();
	{ CNewzflow::CLock lock;
		CNewzflow* theApp = CNewzflow::Instance();
		size_t count = theApp->nzbs.GetCount();
		__int64 eta = 0;
		for(size_t i = 0; i < count; i++) {
			CNzb* nzb = theApp->nzbs[i];
			CString s;
			if((int)i >= lvCount) {
				AddItem(i, kName, nzb->name);
			} else {
				SetItemText(i, kName, nzb->name);
			}
			int image;
			switch(nzb->status) {
			case kQueued:		image = ilQueued; break;
			case kDownloading:	image = ilDownloading; break;
			case kCompleted:	image = ilCompleted; break;
			case kError:		image = ilError; break;
			case kVerifying:	image = ilVerifying; break;
			case kRepairing:	image = ilVerifying; break;
			case kUnpacking:	image = ilVerifying; break;
			case kFinished:		image = ilCompleted; break;
			default:			image = ilQueued; break;
			}
			SetItem(i, kName, LVIF_IMAGE, NULL, image, 0, 0, 0);
			s.Format(_T("%d"), i+1);
			AddItem(i, kOrder, s);
			SetItemData(i, (DWORD_PTR)nzb);
			__int64 completed = 0, total = 0;
			for(size_t j = 0; j < nzb->files.GetCount(); j++) {
				CNzbFile* f = nzb->files[j];
				for(size_t k = 0; k < f->segments.GetCount(); k++) {
					CNzbSegment* s = f->segments[k];
					total += s->bytes;
					if(s->status == kCompleted || s->status == kError)
						completed += s->bytes;
				}
			}
			AddItem(i, kSize, Util::FormatSize(total));
			s.Format(_T("%.1f%%"), 100. * (double)completed / (double)total);
			AddItem(i, kDone, s);
			AddItem(i, kStatus, GetNzbStatusString(nzb->status, nzb->done));
			if(speed > 1024) {
				eta += (total - completed) / speed;
				AddItem(i, kETA, Util::FormatETA(eta));
			} else {
				AddItem(i, kETA, _T("\x221e")); // "unlimited"
			}
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
		CRect rc;
		GetSubItemRect(cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);

		// draw progress frame
		CRect rcProgress(rc);
		rcProgress.DeflateRect(3, 2);
		m_thmProgress.DrawThemeBackground(dcPaint, PP_BAR, 0, rcProgress, NULL);

		CString strItemText;
		GetItemText(cd->nmcd.dwItemSpec, cd->iSubItem, strItemText);

		// draw progress bar															
		rcProgress.DeflateRect(1, 1, 1, 1);
		rcProgress.right = rcProgress.left + (int)( (double)rcProgress.Width() * ((max(min(_tstof(strItemText), 100), 0)) / 100.0));
		m_thmProgress.DrawThemeBackground(dcPaint, PP_CHUNK, 0, rcProgress, NULL);

		m_thmProgress.DrawThemeText(dcPaint, cd->iPartId, cd->iStateId, strItemText, -1, DT_CENTER | DT_SINGLELINE | DT_VCENTER, 0, rc);

		return CDRF_SKIPDEFAULT;
	}
	return CDRF_DODEFAULT;
}

LRESULT CNzbView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CNewzflow::Instance()->settings->SetListViewColumns(_T("NzbView"), *this);

	return 0;
}