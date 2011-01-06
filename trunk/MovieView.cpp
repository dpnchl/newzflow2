// MovieView.cpp : implementation of the CMovieList class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MovieView.h"
#include "Newzflow.h"
#include "Util.h"
#include "Settings.h"
#include "Database.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// TODO: measure height of MovieList/ActorList; don't just add 68 pixels to height of image list

// CMovieList
//////////////////////////////////////////////////////////////////////////

CMovieList::CMovieList()
{
//	SetSortColumn(kDate, false);
}

void CMovieList::Init(HWND hwndParent)
{
	Create(hwndParent, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_ICON | LVS_SHOWSELALWAYS | LVS_SINGLESEL, 0);

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(*this, L"explorer", NULL);

	//AddColumn(_T("Title"), kTitle);

	m_ImageList.Create(width, height, true);

	SetImageList(m_ImageList, LVSIL_NORMAL);
	SetIconSpacing(width+5, height+5);

	Refresh();
}

void CMovieList::Refresh()
{
	SetRedraw(FALSE);
	DeleteAllItems();
	m_ImageList.RemoveAll();
	m_ImageList.AddIcon(LoadIcon(LoadLibrary(_T("shell32.dll")), MAKEINTRESOURCE(1004)));

	QMovies* view = CNewzflow::Instance()->database->GetMovies();
	size_t count = 0;
	while(view->GetRow()) {
		int item = AddItem(count, 0, view->GetTitle(), 0);
		SetItemData(item, view->GetId());

		CString posterPath;
		posterPath.Format(_T("%smovie%d.jpg"), CNewzflow::Instance()->settings->GetAppDataDir(), view->GetTmdbId());

		int image = m_ImageList.Add(posterPath);
		if(image >= 0)
			SetItem(item, 0, LVIF_IMAGE, NULL, image, 0, 0, 0);
		SetItemPosition(item, count * (width+5), 0);

		count++;
	}
	delete view;

	SetRedraw(TRUE);
}

// CActorList
//////////////////////////////////////////////////////////////////////////

CActorList::CActorList()
{
//	SetSortColumn(kDate, false);
}

void CActorList::Init(HWND hwndParent)
{
	Create(hwndParent, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_ICON | LVS_SHOWSELALWAYS, 0);

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(*this, L"explorer", NULL);

	//AddColumn(_T("Title"), kTitle);

	m_ImageList.Create(width, height, true);

	SetImageList(m_ImageList, LVSIL_NORMAL);
	SetIconSpacing(width+5, height+5);
}

void CActorList::SetMovie(int movieId)
{
	SetRedraw(FALSE);
	DeleteAllItems();
	m_ImageList.RemoveAll();
	m_ImageList.AddIcon(LoadIcon(LoadLibrary(_T("shell32.dll")), MAKEINTRESOURCE(1004)), RGB(0xf0, 0xf0, 0xf0) | 0xff000000, RGB(0xd0, 0xd0, 0xd0) | 0xff000000);

	QActors* view = CNewzflow::Instance()->database->GetActors(movieId);
	size_t count = 0;
	while(view->GetRow()) {
		int item = AddItem(count, 0, view->GetName(), 0);
		SetItemData(item, view->GetId());

		CString posterPath;
		posterPath.Format(_T("%sperson%d.jpg"), CNewzflow::Instance()->settings->GetAppDataDir(), view->GetTmdbId());

		int image = m_ImageList.Add(posterPath);
		if(image >= 0)
			SetItem(item, 0, LVIF_IMAGE, NULL, image, 0, 0, 0);
		SetItemPosition(item, count * (width+5), 0);

		count++;
	}
	delete view;
	SetRedraw(TRUE);
}

// CMovieReleaseList
//////////////////////////////////////////////////////////////////////////

// columns
enum {
	kTitle,
	kSize,
	kDate,
};

/*static*/ const CMovieReleaseList::ColumnInfo CMovieReleaseList::s_columnInfo[] = { 
	{ _T("Title"),		_T("Title"),		CMovieReleaseList::typeString,	LVCFMT_LEFT,	400,	true },
	{ _T("Size"),		_T("Size"),			CMovieReleaseList::typeSize,	LVCFMT_RIGHT,	 80,	true },
	{ _T("Date"),		_T("Date"),			CMovieReleaseList::typeDate,	LVCFMT_LEFT,	150,	true },
	{ NULL }
};

CMovieReleaseList::CMovieReleaseList()
{
	m_movieId = 0;
//	SetSortColumn(kDate, false);
}

BOOL CMovieReleaseList::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

void CMovieReleaseList::Init(HWND hwndParent)
{
	Create(hwndParent, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS, 0);

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(*this, L"explorer", NULL);

	InitDynamicColumns(_T("MovieReleaseList"));

	Refresh();
}

int CMovieReleaseList::OnRefresh()
{
	size_t count = 0;
	if(m_movieId) {
		QRssItems* view = CNewzflow::Instance()->database->GetRssItemsByMovie(m_movieId);
		while(view->GetRow()) {
			AddItemEx(count, view->GetId());
			SetItemTextEx(count, kTitle, view->GetTitle());
			SetItemTextEx(count, kSize, Util::FormatSize(view->GetSize()));
			SetItemTextEx(count, kDate, view->GetDate().Format());

			count++;
		}
		delete view;
	}

	return count;
}

void CMovieReleaseList::SetMovie(int movieId)
{
	if(movieId != m_movieId)
		DeleteAllItems();
	m_movieId = movieId;
	Refresh();
}

const CMovieReleaseList::ColumnInfo* CMovieReleaseList::GetColumnInfoArray()
{
	return s_columnInfo;
}

// CCustomDraw
DWORD CMovieReleaseList::OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
{
	return CDRF_NOTIFYITEMDRAW;
}

DWORD CMovieReleaseList::OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
{
	return CDRF_NOTIFYSUBITEMDRAW;
}

DWORD CMovieReleaseList::OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW lpNMCustomDraw)
{
	return CDRF_DODEFAULT;
}

// CMovieView
//////////////////////////////////////////////////////////////////////////

void CMovieView::Init(HWND hwndParent)
{
	Create(hwndParent, rcDefault, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_ICON | LVS_SHOWSELALWAYS, 0);
	m_MovieList.Init(*this);
	m_ActorList.Init(*this);
	m_ReleaseList.Init(*this);
}

void CMovieView::Refresh()
{
	m_MovieList.Refresh();
}

void CMovieView::OnSize(UINT nType, CSize size)
{
	HDWP hdwp = BeginDeferWindowPos(2);
	int y = 0;
	if(m_MovieList.IsWindow()) {
		int height = CMovieList::height + 58;
		m_MovieList.DeferWindowPos(hdwp, NULL, 0, 0, size.cx, height, SWP_NOZORDER | SWP_NOACTIVATE);
		y += height;
	}
	if(m_ActorList.IsWindow()) {
		int height = CActorList::height + 58;
		m_ActorList.DeferWindowPos(hdwp, NULL, 0, y, size.cx, height, SWP_NOZORDER | SWP_NOACTIVATE);
		y += height;
	}
	if(m_ReleaseList.IsWindow()) {
		int height = max(0, (int)size.cy - y);
		m_ReleaseList.DeferWindowPos(hdwp, NULL, 0, y, size.cx, height, SWP_NOZORDER | SWP_NOACTIVATE);
		y += height;
	}
	EndDeferWindowPos(hdwp);
}

LRESULT CMovieView::OnItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	LPNMLISTVIEW nmlv = (LPNMLISTVIEW)pnmh;
	if(pnmh->hwndFrom == m_MovieList) {
		if(nmlv->uNewState & LVIS_SELECTED) {
			m_ActorList.SetMovie(nmlv->lParam);
			m_ReleaseList.SetMovie(nmlv->lParam);
		}
	}
	return 0;
}

LRESULT CMovieView::OnDblClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	LPNMLISTVIEW nmlv = (LPNMLISTVIEW)pnmh;
	if(pnmh->hwndFrom == m_ReleaseList) {
		if(m_ReleaseList.GetNextItem(-1, LVNI_SELECTED) != -1) {
			{ CTransaction transaction(CNewzflow::Instance()->database);
				for(int iItem = m_ReleaseList.GetNextItem(-1, LVNI_SELECTED); iItem != -1; iItem = m_ReleaseList.GetNextItem(iItem, LVNI_SELECTED)) {
					int id = m_ReleaseList.GetItemData(iItem);

					// get URL for RSS item and add NZB; set status to downloaded
					CString sUrl = CNewzflow::Instance()->database->DownloadRssItem(id);
					if(!sUrl.IsEmpty())
						CNewzflow::Instance()->controlThread->AddURL(sUrl);
					else
						transaction.Rollback();
				}
			}
		}
	}
	return 0;
}