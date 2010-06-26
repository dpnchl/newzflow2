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

/*static*/ const CNzbView::ColumnInfo CNzbView::s_columnInfo[] = { 
	{ _T("Name"),		_T("Name"),		CNzbView::typeString,	LVCFMT_LEFT,	400,	true },
	{ _T("#"),			_T("#"),		CNzbView::typeNumber,	LVCFMT_RIGHT,	20,		true },
	{ _T("Size"),		_T("Size"),		CNzbView::typeSize,		LVCFMT_RIGHT,	80,		true },
	{ _T("Done"),		_T("Done"),		CNzbView::typeNumber,	LVCFMT_RIGHT,	80,		true },
	{ _T("Status"),		_T("Status"),	CNzbView::typeString,	LVCFMT_LEFT,	120,	true },
	{ _T("ETA"),		_T("ETA"),		CNzbView::typeTimeSpan,	LVCFMT_LEFT,	80,		true },
	{ NULL }
};

CNzbView::CNzbView()
{
}

CNzbView::~CNzbView()
{
	m_imageList.Destroy();
}

BOOL CNzbView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

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

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(*this, L"explorer", NULL);

	InitDynamicColumns(_T("NzbView"));
}

int CNzbView::OnRefresh()
{
	int speed = CNntpSocket::totalSpeed.Get();
	{ CNewzflow::CLock lock;
		CNewzflow* theApp = CNewzflow::Instance();
		size_t count = theApp->nzbs.GetCount();
		__int64 eta = 0;

		// insert everything again, restoring state if available
		//SetItemCount(count);
		for(size_t i = 0; i < count; i++) {
			CString s;
			CNzb* nzb = theApp->nzbs[i];
			AddItemEx(i, (DWORD_PTR)nzb);
			SetItemTextEx(i, kName, nzb->name);
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
			SetItemEx(i, kName, LVIF_IMAGE, NULL, image, 0, 0, 0);
			s.Format(_T("%d"), i+1);
			SetItemTextEx(i, kOrder, s);
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
			SetItemTextEx(i, kSize, Util::FormatSize(total));
			s.Format(_T("%.1f%%"), 100. * (double)completed / (double)total);
			SetItemTextEx(i, kDone, s);
			SetItemTextEx(i, kStatus, GetNzbStatusString(nzb->status, nzb->done));
			if(speed > 1024) {
				eta += (total - completed) / speed;
				SetItemTextEx(i, kETA, Util::FormatTimeSpan(eta));
			} else {
				SetItemTextEx(i, kETA, _T("\x221e")); // "unlimited"
			}
		}
		return count;
	}
}

LRESULT CNzbView::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	Refresh();

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
	if(cd->iSubItem == SubItemFromColumn(kDone)) {
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

const CDynamicColumns<CNzbView>::ColumnInfo* CNzbView::GetColumnInfoArray()
{
	return s_columnInfo;
}

#if 0
	BOOL b;
	OnTimer(WM_TIMER, 0, 0, b);

	return 0;
#endif