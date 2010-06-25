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
	kMax,
};

/*static*/ const CNzbView::ColumnInfo CNzbView::s_columnInfo[] = { 
	{ _T("Name"),		_T("Name"),		CNzbView::typeString,	LVCFMT_LEFT,	400,	true },
	{ _T("#"),			_T("#"),		CNzbView::typeNumber,	LVCFMT_RIGHT,	20,		true },
	{ _T("Size"),		_T("Size"),		CNzbView::typeSize,		LVCFMT_RIGHT,	80,		true },
	{ _T("Done"),		_T("Done"),		CNzbView::typeNumber,	LVCFMT_RIGHT,	80,		true },
	{ _T("Status"),		_T("Status"),	CNzbView::typeString,	LVCFMT_LEFT,	120,	true },
	{ _T("ETA"),		_T("ETA"),		CNzbView::typeTimeSpan,	LVCFMT_LEFT,	80,		true },
};

CNzbView::CNzbView()
{
	m_columns = NULL;
	m_lockUpdate = false;
	m_sortColumn = kOrder;
	m_sortAsc = true;
}

CNzbView::~CNzbView()
{
	delete m_columns;
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

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP);
	SetWindowTheme(*this, L"explorer", NULL);
	for(int i = 0; i < kMax; i++) {
		AddColumn(s_columnInfo[i].nameShort, i, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, s_columnInfo[i].format);
		SetColumnWidth(i, s_columnInfo[i].width);
	}

	m_columns = new int[kMax];
	for(int i = 0; i < kMax; i++)
		m_columns[i] = i;

	CNewzflow::Instance()->settings->GetListViewColumns(_T("NzbView"), *this, m_columns, kMax);
}

struct CompareData
{
	CListViewCtrl lv;
	int column;
	bool asc;
	CNzbView::EColumnType type;

	static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
	{
		CompareData* compareData = (CompareData*)lParamSort;

		CString s1, s2;
		compareData->lv.GetItemText(lParam1, compareData->column, s1);
		compareData->lv.GetItemText(lParam2, compareData->column, s2);

		int ret = 0;

		switch(compareData->type) {
		case CNzbView::typeString: {
			ret = s1.CompareNoCase(s2); 
			break;
		}
		case CNzbView::typeNumber: {
			double f1 = _tstof(s1);
			double f2 = _tstof(s2);
			if(f1 < f2) ret = -1;
			else if(f1 == f2) ret = 0;
			else ret = 1;
			break;
		}
		case CNzbView::typeSize: {
			__int64 size1 = Util::ParseSize(s1);
			__int64 size2 = Util::ParseSize(s2);
			if(size1 < size2) ret = -1;
			else if(size1 == size2) ret = 0;
			else ret = 1;
		}
		case CNzbView::typeTimeSpan: {
			__int64 span1 = Util::ParseTimeSpan(s1);
			__int64 span2 = Util::ParseTimeSpan(s2);
			if(span1 < span2) ret = -1;
			else if(span1 == span2) ret = 0;
			else ret = 1;
		}
		}

		if(!compareData->asc) ret = -ret;

		return ret;
	}
};

LRESULT CNzbView::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if(m_lockUpdate)
		return 0;


	int speed = CNntpSocket::totalSpeed.Get();
	{ CNewzflow::CLock lock;
		CNewzflow* theApp = CNewzflow::Instance();
		size_t count = theApp->nzbs.GetCount();
		__int64 eta = 0;
		SetRedraw(FALSE);

		// save state (focus/selection/hot) for list
		CAtlMap<CNzb*, int> stateMap;
		int lvCount = GetItemCount();
		for(int i = 0; i < lvCount; i++) {
			stateMap[(CNzb*)GetItemData(i)] = GetItemState(i, LVIS_FOCUSED | LVIS_SELECTED);
		}
		int hot = GetHotItem();
		// delete all items
		DeleteAllItems();
		// insert everything again, restoring state if available
		SetItemCount(count);
		for(size_t i = 0; i < count; i++) {
			CString s;
			CNzb* nzb = theApp->nzbs[i];
			AddItem(i, 0, _T(""));
			int state;
			if(stateMap.Lookup(nzb, state))
				SetItemState(i, state, LVIS_FOCUSED | LVIS_SELECTED);
			SetItemData(i, (DWORD_PTR)nzb);
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
		SetHotItem(hot);

		CompareData compareData = { *this, m_sortColumn, m_sortAsc, s_columnInfo[m_sortColumn].type };
		SortItemsEx(CompareData::CompareFunc, (LPARAM)&compareData);
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
	if(cd->iSubItem == m_columns[kDone]) {
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
	CNewzflow::Instance()->settings->SetListViewColumns(_T("NzbView"), *this, m_columns, kMax);

	return 0;
}

LRESULT CNzbView::OnHdnItemClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	LPNMHEADER lpnm = (LPNMHEADER)pnmh;

	for(int i = 0; i < GetHeader().GetItemCount(); i++) {
		HDITEM hditem = {0};
		hditem.mask = HDI_FORMAT;
		GetHeader().GetItem(i, &hditem);
		if (i == lpnm->iItem) {
			if(hditem.fmt & HDF_SORTDOWN)
				hditem.fmt = (hditem.fmt & ~HDF_SORTDOWN) | HDF_SORTUP;
			else
				hditem.fmt = (hditem.fmt & ~HDF_SORTUP) | HDF_SORTDOWN;
		} else {
			hditem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
		}
		GetHeader().SetItem(i, &hditem);
	}
	SetSelectedColumn(lpnm->iItem);
	if(m_sortColumn == lpnm->iItem)
		m_sortAsc = !m_sortAsc;
	else {
		m_sortColumn = lpnm->iItem;
		m_sortAsc = true;
	}

	CompareData compareData = { *this, m_sortColumn, m_sortAsc, s_columnInfo[m_sortColumn].type };
	SortItemsEx(CompareData::CompareFunc, (LPARAM)&compareData);

	return 0;
}

BOOL CNzbView::SetItemEx(int nItem, int nSubItem, UINT nMask, LPCTSTR lpszItem, int nImage, UINT nState, UINT nStateMask, LPARAM lParam)
{
	if(m_columns[nSubItem] < 0)
		return TRUE;
	return SetItem(nItem, m_columns[nSubItem], nMask, lpszItem, nImage, nState, nStateMask, lParam);
}

BOOL CNzbView::SetItemTextEx(int nItem, int nSubItem, LPCTSTR lpszText)
{
	if(m_columns[nSubItem] < 0)
		return TRUE;
	return SetItemText(nItem, m_columns[nSubItem], lpszText);
}

LRESULT CNzbView::OnHdnRClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	const DWORD pos = GetMessagePos();
	CPoint pt(GET_X_LPARAM(pos), GET_Y_LPARAM(pos));

	CMenu menu;
	menu.CreatePopupMenu();
	for(int i = 0; i < kMax; i++) {
		menu.AppendMenu((m_columns[i] >= 0 ? MF_CHECKED : 0) | MF_STRING, i+1, s_columnInfo[i].nameLong);
	}
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, 10000, _T("Reset"));

	int ret = (int)menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, *this);
	if(ret == 0) // cancelled
		return 0;

	m_lockUpdate = true;
	SetRedraw(FALSE);

	if(ret < kMax+1) {
		int column = ret - 1;
		if(m_columns[column] >= 0) {
			// hide column
			int col = m_columns[column];
			m_columns[column] = -1;
			for(int i = 0; i < kMax; i++) {
				if(m_columns[i] >= column)
					m_columns[i]--;
			}
			DeleteColumn(col);
		} else {
			// show column
			for(int i = 0; i < kMax; i++) {
				if(m_columns[i] >= column)
					m_columns[i]++;
			}
			m_columns[column] = column;
			InsertColumn(column, s_columnInfo[column].nameShort, s_columnInfo[column].format, -1);
			SetColumnWidth(column, s_columnInfo[column].width);
		}
	} else if(ret == 10000) {
		// reset
		int numCols = GetHeader().GetItemCount();
		for(int i = 0; i < numCols; i++)
			DeleteColumn(0);
		for(int i = 0; i < kMax; i++) {
			m_columns[i] = i;
			AddColumn(s_columnInfo[i].nameShort, i, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, s_columnInfo[i].format);
			SetColumnWidth(i, s_columnInfo[i].width);
		}
	}

	m_lockUpdate = false;
	SetRedraw(TRUE);
	BOOL b;
	OnTimer(WM_TIMER, 0, 0, b);

	return 0;
}