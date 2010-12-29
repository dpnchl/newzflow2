// RssView.cpp : implementation of the CRssView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "RssView.h"
#include "Newzflow.h"
#include "Util.h"
#include "Settings.h"
#include "Database.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// columns
enum {
	kTitle,
	kSize,
	kFeed,
	kDate,
	kStatus,
};

/*static*/ const CRssView::ColumnInfo CRssView::s_columnInfo[] = { 
	{ _T("Title"),		_T("Title"),		CRssView::typeString,	LVCFMT_LEFT,	400,	true },
	{ _T("Size"),		_T("Size"),			CRssView::typeSize,		LVCFMT_RIGHT,	80,		true },
	{ _T("Feed"),		_T("Feed"),			CRssView::typeString,	LVCFMT_LEFT,	150,	true },
	{ _T("Date"),		_T("Date"),			CRssView::typeDate,		LVCFMT_LEFT,	150,	true },
	{ _T("Status"),		_T("Status"),		CRssView::typeString,	LVCFMT_LEFT,	120,	true },
	{ NULL }
};

CRssView::CRssView()
{
	m_feedId = 0;
	SetSortColumn(kDate, false);
}

BOOL CRssView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

void CRssView::Init(HWND hwndParent)
{
	Create(hwndParent, rcDefault, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS, 0);

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(*this, L"explorer", NULL);

	InitDynamicColumns(_T("RssView"));

	Refresh();
}

int CRssView::OnRefresh()
{
	QRssItems* rssView = CNewzflow::Instance()->database->GetRssItems(m_feedId);
	size_t count = 0;
	while(rssView->GetRow()) {
		AddItemEx(count, rssView->GetId());
		SetItemTextEx(count, kTitle, rssView->GetTitle());
		SetItemTextEx(count, kSize, Util::FormatSize(rssView->GetSize()));
		SetItemTextEx(count, kFeed, rssView->GetFeedTitle());
		SetItemTextEx(count, kDate, rssView->GetDate().Format());
		CString s;
		switch(rssView->GetStatus()) {
		case QRssItems::kDownloaded: s = _T("Downloaded"); break;
		}
		SetItemTextEx(count, kStatus, s);

		count++;
	}
	delete rssView;

	return count;
}

LRESULT CRssView::OnDblClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
	if(GetNextItem(-1, LVNI_SELECTED) != -1) {
		{ CTransaction transaction(CNewzflow::Instance()->database);
			for(int iItem = GetNextItem(-1, LVNI_SELECTED); iItem != -1; iItem = GetNextItem(iItem, LVNI_SELECTED)) {
				int id = GetItemData(iItem);

				// get URL for RSS item and add NZB; set status to downloaded
				CString sUrl = CNewzflow::Instance()->database->DownloadRssItem(id);
				if(!sUrl.IsEmpty())
					CNewzflow::Instance()->controlThread->AddURL(sUrl);
				else
					transaction.Rollback();
			}
		}
		Refresh();
	}
	return 0;
}

// feedId = 0 => all feeds; otherwise feedId = RssFeeds.rowid
void CRssView::SetFeed(int feedId)
{
	if(feedId != m_feedId)
		DeleteAllItems();
	m_feedId = feedId;
	Refresh();
}

const CRssView::ColumnInfo* CRssView::GetColumnInfoArray()
{
	return s_columnInfo;
}

// CCustomDraw
DWORD CRssView::OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
{
	return CDRF_NOTIFYITEMDRAW;
}

DWORD CRssView::OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
{
	return CDRF_NOTIFYSUBITEMDRAW;
}

DWORD CRssView::OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW lpNMCustomDraw)
{
	return CDRF_DODEFAULT;
}
