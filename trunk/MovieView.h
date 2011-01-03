// MovieView.h : interface of the CNewzflowView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CNzb;

#include "ListViewEx.h"
#include "Util.h"

class CMovieList : public CWindowImpl<CMovieList, CListViewCtrl>
{
public:
	DECLARE_WND_SUPERCLASS(_T("CMovieList"), CListViewCtrl::GetWndClassName())

	BEGIN_MSG_MAP(CMovieList)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	CMovieList();

	void Init(HWND hwndParent);
	void Refresh();

protected:
	CImageListEx m_ImageList;

	enum {
		width = 166,
		height = 250
	};

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
};

class CActorList : public CWindowImpl<CActorList, CListViewCtrl>
{
public:
	DECLARE_WND_SUPERCLASS(_T("CActorList"), CListViewCtrl::GetWndClassName())

	BEGIN_MSG_MAP(CActorList)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	CActorList();

	void Init(HWND hwndParent);
	void SetMovie(int movieId);

protected:
	CImageListEx m_ImageList;

	enum {
		width = 92,
		height = 139
	};
};

class CMovieView : public CWindowImpl<CMovieView>
{
public:
	BEGIN_MSG_MAP(CMovieView)
		NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
		//REFLECTED_NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDblClick)
		//DEFAULT_REFLECTION_HANDLER()
		MSG_WM_SIZE(OnSize)
	END_MSG_MAP()

	void Init(HWND hwndParent);

	void OnSize(UINT nType, CSize size);
	LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

protected:
	CMovieList m_MovieList;
	CActorList m_ActorList;
};