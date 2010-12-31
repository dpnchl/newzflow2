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

// TODO: extract image scaling code; proper aspect-keeping scaling; smaller size for actors


// CMovieList
//////////////////////////////////////////////////////////////////////////

// columns
enum {
	kTitle,
	kEpisode,
	kDate,
};


CMovieList::CMovieList()
{
//	SetSortColumn(kDate, false);
}

void CMovieList::Init(HWND hwndParent)
{
	Create(hwndParent, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_ICON | LVS_SHOWSELALWAYS | LVS_SINGLESEL, 0);

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(*this, L"explorer", NULL);

	AddColumn(_T("Title"), kTitle);

	m_ImageList.Create(185, 278, ILC_COLOR32, 0, 200);
	CImage imgEmpty;
	imgEmpty.Create(185, 278, 32);
	unsigned char* bits = (unsigned char*)imgEmpty.GetBits();
	int pitch = imgEmpty.GetPitch();
	for(int y = 0; y < imgEmpty.GetHeight(); y++) {
		memset(bits, 255, imgEmpty.GetWidth() * imgEmpty.GetBPP() / 8);
		bits += pitch;
	}
	m_ImageList.Add((HBITMAP)imgEmpty, (HBITMAP)NULL);

	SetImageList(m_ImageList, LVSIL_NORMAL);
	SetIconSpacing(185+5, 278+5);

	Refresh();
}

void CMovieList::Refresh()
{
	DeleteAllItems();

	QMovies* view = CNewzflow::Instance()->database->GetMovies();
	size_t count = 0;
	while(view->GetRow()) {
		int item = AddItem(count, 0, _T(""), 0);
		SetItemData(item, view->GetId());
		SetItemText(count, kTitle, view->GetTitle());

		CString posterPath;
		posterPath.Format(_T("%smovie%d.jpg"), CNewzflow::Instance()->settings->GetAppDataDir(), view->GetTmdbId());

		CImage image;
		if(SUCCEEDED(image.Load(posterPath))) {
			// resize image to 185x278
			int width=185,height=278;
			CImage img1; 
			img1.Create(width,height,32); 
			CDC dc; 
			dc.Attach(img1.GetDC()); 
			dc.SetStretchBltMode(HALFTONE); 
			image.StretchBlt(dc,0,0,width,height,SRCCOPY); 
			dc.Detach(); 
			img1.ReleaseDC(); 
			int imageId = m_ImageList.GetImageCount();
			m_ImageList.Add((HBITMAP)img1, (HBITMAP)NULL);
			SetItem(item, 0, LVIF_IMAGE, NULL, imageId, 0, 0, 0);
		}
		SetItemPosition(item, count * (185+5), 0);

		count++;
	}
	delete view;
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

	AddColumn(_T("Title"), kTitle);

	m_ImageList.Create(185, 278, ILC_COLOR32, 0, 200);

	SetImageList(m_ImageList, LVSIL_NORMAL);
	SetIconSpacing(185+5, 278+5);
}

void CActorList::SetMovie(int movieId)
{
	SetRedraw(FALSE);
	DeleteAllItems();

	m_ImageList.RemoveAll();
	CImage imgEmpty;
	imgEmpty.Create(185, 278, 32);
	unsigned char* bits = (unsigned char*)imgEmpty.GetBits();
	int pitch = imgEmpty.GetPitch();
	for(int y = 0; y < imgEmpty.GetHeight(); y++) {
		memset(bits, 255, imgEmpty.GetWidth() * imgEmpty.GetBPP() / 8);
		bits += pitch;
	}
	m_ImageList.Add((HBITMAP)imgEmpty, (HBITMAP)NULL);

	QActors* view = CNewzflow::Instance()->database->GetActors(movieId);
	size_t count = 0;
	while(view->GetRow()) {
		int item = AddItem(count, 0, _T(""), 0);
		SetItemData(item, view->GetId());
		SetItemText(count, kTitle, view->GetName());

		CString posterPath;
		posterPath.Format(_T("%sperson%d.jpg"), CNewzflow::Instance()->settings->GetAppDataDir(), view->GetTmdbId());

		CImage image;
		if(SUCCEEDED(image.Load(posterPath))) {
			// resize image to 185x278
			int width=185,height=278;
			CImage img1; 
			img1.Create(width,height,32); 
			CDC dc; 
			dc.Attach(img1.GetDC()); 
			dc.SetStretchBltMode(HALFTONE); 
			image.StretchBlt(dc,0,0,width,height,SRCCOPY); 
			dc.Detach(); 
			img1.ReleaseDC(); 
			int imageId = m_ImageList.GetImageCount();
			m_ImageList.Add((HBITMAP)img1, (HBITMAP)NULL);
			SetItem(item, 0, LVIF_IMAGE, NULL, imageId, 0, 0, 0);
		}
		SetItemPosition(item, count * (185+5), 0);

		count++;
	}
	delete view;
	SetRedraw(TRUE);
}

// CMovieView
//////////////////////////////////////////////////////////////////////////

void CMovieView::Init(HWND hwndParent)
{
	Create(hwndParent, rcDefault, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_ICON | LVS_SHOWSELALWAYS, 0);
	m_MovieList.Init(*this);
	m_ActorList.Init(*this);
}

void CMovieView::OnSize(UINT nType, CSize size)
{
	HDWP hdwp = BeginDeferWindowPos(2);
	if(m_MovieList.IsWindow())
		m_MovieList.DeferWindowPos(hdwp, NULL, 0, 0, size.cx, size.cy/2, SWP_NOZORDER | SWP_NOACTIVATE);
	if(m_ActorList.IsWindow())
		m_ActorList.DeferWindowPos(hdwp, NULL, 0, size.cy/2, size.cx, size.cy/2, SWP_NOZORDER | SWP_NOACTIVATE);
	EndDeferWindowPos(hdwp);
}

LRESULT CMovieView::OnItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	LPNMLISTVIEW nmlv = (LPNMLISTVIEW)pnmh;
	if(pnmh->hwndFrom == m_MovieList) {
		if(nmlv->uNewState & LVIS_SELECTED) {
			m_ActorList.SetMovie(nmlv->lParam);
		}
	}
	return 0;
}