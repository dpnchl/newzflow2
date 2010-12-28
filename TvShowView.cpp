// TvShowView.cpp : implementation of the CTvShowView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "TvShowView.h"
#include "Newzflow.h"
#include "Util.h"
#include "Settings.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// columns
enum {
	kTitle,
	kEpisode,
	kDate,
};

/*static*/ const CTvShowView::ColumnInfo CTvShowView::s_columnInfo[] = { 
	{ _T("Title"),		_T("Title"),		CTvShowView::typeString,	LVCFMT_LEFT,	400,	true },
	{ _T("Episode"),	_T("Ep"),			CTvShowView::typeNumber,	LVCFMT_RIGHT,	 40,	true },
	{ _T("Date"),		_T("Date"),			CTvShowView::typeDate,		LVCFMT_LEFT,	 70,	true },
	{ NULL }
};

CTvShowView::CTvShowView()
{
	m_showId = 0;
//	SetSortColumn(kDate, false);
}

BOOL CTvShowView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

void CTvShowView::Init(HWND hwndParent)
{
	Create(hwndParent, rcDefault, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS, 0);

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(*this, L"explorer", NULL);
	EnableGroupView(TRUE);

	InitDynamicColumns(_T("TvShowView"));

	Refresh();
}

namespace {
	int CALLBACK GroupCompare(int group1, int group2, void *param)
	{
		return group2 - group1;
	}
}


int CTvShowView::OnRefresh()
{
	CString sQuery(_T("SELECT rowid, season, episode, title, strftime('%s', date) FROM TvEpisodes WHERE show_id = ? ORDER BY episode DESC"));
	sq3::Statement st(CNewzflow::Instance()->database, sQuery);
	if(!st.IsValid())
		TRACE(_T("DB error: %s\n"), CNewzflow::Instance()->database.GetErrorMessage());
	st.Bind(0, m_showId);
	sq3::Reader reader = st.ExecuteReader();
	CAtlMap<int, int> seasonGroups;
	size_t count = 0;
	while(reader.Step() == SQLITE_ROW) {
		int id; reader.GetInt(0, id);
		int season; reader.GetInt(1, season);
		int episode; reader.GetInt(2, episode);
		CString sTitle; reader.GetString(3, sTitle);
		int date; reader.GetInt(4, date);

		CString sEpisode; sEpisode.Format(_T("%d"), episode);

		AddItemEx(count, id);
		SetItemTextEx(count, kEpisode, sEpisode);
		SetItemTextEx(count, kTitle, sTitle);
		SetItemTextEx(count, kDate, date != 0 ? COleDateTime((time_t)date).Format(VAR_DATEVALUEONLY) : _T("never"));

		int groupId = -1;
		// create group
		if(!seasonGroups.Lookup(season, groupId)) {
			LVGROUP lg = { 0 };
			lg.cbSize = LVGROUP_V5_SIZE; // for XP compatibility
			lg.iGroupId = season;
			lg.mask = LVGF_GROUPID | LVGF_HEADER | LVGF_STATE;
			lg.state = LVGS_NORMAL;
			CStringW title;
			if(season == 0)
				title = L"Specials";
			else
				title.Format(L"Season %d", season);
			lg.pszHeader = (LPWSTR)(LPCWSTR)title;
			lg.cchHeader = title.GetLength();
			groupId = AddGroup(&lg);
			groupId = season;

			seasonGroups[season] = groupId;
		}
		// set group
		LVITEM lvi = { 0 };
		lvi.mask = LVIF_GROUPID;
		lvi.iItem = count;
		lvi.iSubItem = 0;
		lvi.iGroupId = groupId;
		SetItem(&lvi);

		count++;
	}

	SortGroups(GroupCompare);

	return count;
}

LRESULT CTvShowView::OnDblClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
/*	if(GetNextItem(-1, LVNI_SELECTED) != -1) {
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
*/	return 0;
}

void CTvShowView::SetShow(int showId)
{
	if(showId != m_showId)
		DeleteAllItems();
	m_showId = showId;
	Refresh();
}

const CTvShowView::ColumnInfo* CTvShowView::GetColumnInfoArray()
{
	return s_columnInfo;
}

// CCustomDraw
DWORD CTvShowView::OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
{
	return CDRF_NOTIFYITEMDRAW;
}

DWORD CTvShowView::OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
{
	return CDRF_NOTIFYSUBITEMDRAW;
}

DWORD CTvShowView::OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW lpNMCustomDraw)
{
	return CDRF_DODEFAULT;
}
