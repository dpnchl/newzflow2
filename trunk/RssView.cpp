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

/*static*/ const CRssView::ColumnInfo CRssView::s_columnInfo[] = { 
	{ _T("Title"),		_T("Title"),		CRssView::typeString,	LVCFMT_LEFT,	400,	true },
	{ _T("Size"),		_T("Size"),			CRssView::typeSize,		LVCFMT_RIGHT,	80,		true },
	{ _T("Feed"),		_T("Feed"),			CRssView::typeString,	LVCFMT_LEFT,	150,	true },
	{ _T("Date"),		_T("Date"),			CRssView::typeTimeSpan,	LVCFMT_LEFT,	150,	true }, // TODO: typeDate
	{ _T("Status"),		_T("Status"),		CRssView::typeString,	LVCFMT_LEFT,	120,	true },
	{ NULL }
};

CRssView::CRssView()
{
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
	// TODO: add column names to Reader/Statement
	sq3::Statement st(CNewzflow::Instance()->database, _T("SELECT RssItems.rowid, RssItems.title, RssItems.length, strftime('%s', RssItems.date), RssFeeds.name FROM RssItems LEFT JOIN RssFeeds ON RssItems.feed = RssFeeds.rowid ORDER BY date DESC"));
	if(!st.IsValid())
		TRACE(_T("DB error: %s\n"), CNewzflow::Instance()->database.GetErrorMessage());
	sq3::Reader reader = st.ExecuteReader();
	size_t count = 0;
	while(reader.Step() == SQLITE_ROW) {
		__int64 id; reader.GetInt64(0, id);
		CString sTitle; reader.GetString(1, sTitle);
		__int64 length; reader.GetInt64(2, length);
		int date; reader.GetInt(3, date);
		CString sFeed; reader.GetString(4, sFeed);

		AddItemEx(count, (DWORD_PTR)id); // TODO: don#t just truncate the 64-bit ID, store something else
		SetItemTextEx(count, kTitle, sTitle);
		SetItemTextEx(count, kSize, Util::FormatSize(length));
		SetItemTextEx(count, kFeed, sFeed);
		SetItemTextEx(count, kDate, COleDateTime((time_t)date).Format());

		count++;
	}
	return count;
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
