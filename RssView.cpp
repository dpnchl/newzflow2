// RssView.cpp : implementation of the CRssView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "RssView.h"
#include "Newzflow.h"
#include "Util.h"
#include "Settings.h"

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

// for 'status'
enum {
	kDownloaded = 1,
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
	CString sQuery(_T("SELECT RssItems.rowid, RssItems.title, RssItems.length, strftime('%s', RssItems.date), RssItems.status, RssFeeds.title FROM RssItems LEFT JOIN RssFeeds ON RssItems.feed = RssFeeds.rowid"));
	if(m_feedId > 0)
		sQuery += _T(" WHERE RssFeeds.rowid = ?");

	sq3::Statement st(CNewzflow::Instance()->database, sQuery);
	if(!st.IsValid())
		TRACE(_T("DB error: %s\n"), CNewzflow::Instance()->database.GetErrorMessage());
	if(m_feedId > 0)
		st.Bind(0, m_feedId);
	sq3::Reader reader = st.ExecuteReader();
	size_t count = 0;
	while(reader.Step() == SQLITE_ROW) {
		int id; reader.GetInt(0, id);
		CString sTitle; reader.GetString(1, sTitle);
		__int64 length; reader.GetInt64(2, length);
		int date; reader.GetInt(3, date);
		int status; reader.GetInt(4, status);
		CString sFeed; reader.GetString(5, sFeed);

		AddItemEx(count, id);
		SetItemTextEx(count, kTitle, sTitle);
		SetItemTextEx(count, kSize, Util::FormatSize(length));
		SetItemTextEx(count, kFeed, sFeed);
		SetItemTextEx(count, kDate, COleDateTime((time_t)date).Format());
		CString s;
		switch(status) {
		case kDownloaded: s = _T("Downloaded"); break;
		}
		SetItemTextEx(count, kStatus, s);

		count++;
	}
	return count;
}

LRESULT CRssView::OnDblClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
	if(GetNextItem(-1, LVNI_SELECTED) != -1) {
		{ sq3::Transaction transaction(CNewzflow::Instance()->database);
			for(int iItem = GetNextItem(-1, LVNI_SELECTED); iItem != -1; iItem = GetNextItem(iItem, LVNI_SELECTED)) {
				int id = GetItemData(iItem);

				// get URL for RSS item and add NZB
				{
					sq3::Statement st(CNewzflow::Instance()->database, _T("SELECT link FROM RssItems WHERE rowid = ?"));
					st.Bind(0, id);
					CString sUrl;
					if(st.ExecuteString(sUrl) != SQLITE_OK) {
						TRACE(_T("DB error: %s\n"), CNewzflow::Instance()->database.GetErrorMessage());
					}
					CNewzflow::Instance()->controlThread->AddURL(sUrl);
				}

				// set status to downloaded
				{
					sq3::Statement st(CNewzflow::Instance()->database, _T("UPDATE RssItems SET status = ? WHERE rowid = ?"));
					st.Bind(0, kDownloaded);
					st.Bind(1, id);
					if(st.ExecuteNonQuery() != SQLITE_OK) {
						TRACE(_T("DB error: %s\n"), CNewzflow::Instance()->database.GetErrorMessage());
					}
				}
			}
			transaction.Commit();
		}
		Refresh();
	}
	return 0;
}

// feedId = 0 => all feeds; otherwise feedId = RssFeeds.rowid
void CRssView::SetFeed(int feedId)
{
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
