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

	enum {
		width = 166,
		height = 250
	};

protected:
	CImageListEx m_ImageList;

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

	enum {
		width = 92,
		height = 139
	};

protected:
	CImageListEx m_ImageList;
};

class CMovieReleaseList : public CWindowImpl<CMovieReleaseList, CListViewCtrl>, public CDynamicColumns<CMovieReleaseList>, public CCustomDraw<CMovieReleaseList>
{
public:
	DECLARE_WND_SUPERCLASS(_T("CMovieReleaseList"), CListViewCtrl::GetWndClassName())

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CMovieReleaseList)
		CHAIN_MSG_MAP(CDynamicColumns<CMovieReleaseList>)
		CHAIN_MSG_MAP_ALT(CCustomDraw<CMovieReleaseList>, 1)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	CMovieReleaseList();

	void Init(HWND hwndParent);
	void SetMovie(int movieId);

	// CCustomDraw overrides
	DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);

	// CSortableList overrides
	int OnRefresh();
	const ColumnInfo* GetColumnInfoArray();

protected:
	static const ColumnInfo s_columnInfo[];

	int m_movieId;
};

class CMovieView : public CWindowImpl<CMovieView>
{
public:
	BEGIN_MSG_MAP(CMovieView)
		NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
		NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDblClick)
		//DEFAULT_REFLECTION_HANDLER()
		MSG_WM_SIZE(OnSize)
	END_MSG_MAP()

	void Init(HWND hwndParent);
	void Refresh();

protected:
	void OnSize(UINT nType, CSize size);
	LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnDblClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

protected:
	CMovieList m_MovieList;
	CActorList m_ActorList;
	CMovieReleaseList m_ReleaseList;
};