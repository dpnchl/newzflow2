// ViewTree.h : interface of the CNewzflowView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CNzb;

class CViewTree : public CWindowImpl<CViewTree, CTreeViewCtrlEx>
{
public:
	DECLARE_WND_SUPERCLASS(_T("CViewTree"), CTreeViewCtrlEx::GetWndClassName())

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CViewTree)
		REFLECTED_NOTIFY_CODE_HANDLER(NM_RCLICK, OnRClick)
		COMMAND_ID_HANDLER(ID_FEEDS_ADD, OnFeedsAdd)
		COMMAND_ID_HANDLER(ID_FEEDS_EDIT, OnFeedsEdit)
		COMMAND_ID_HANDLER(ID_FEEDS_DELETE, OnFeedsDelete)
		COMMAND_ID_HANDLER(ID_TV_ADD, OnTvAdd)
		COMMAND_ID_HANDLER(ID_TV_DELETE, OnTvDelete)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	CViewTree();

	void Init(HWND hwndParent);
	void Refresh();

	LRESULT OnRClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnFeedsAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFeedsEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFeedsDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnTvAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnTvDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSelChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	CTreeItem tvDownloads, tvFeeds, tvTvShows, tvMovies;

	enum {
		kDownloads,
		kFeeds, // kFeeds+0 => all feeds; kFeeds+feedId => single feed (id Id)
		kFeedsEnd = kFeeds + 100000,
		kTvShows,
		kTvShowsEnd = kTvShows + 100000,
		kMovies,
	};

protected:
	CImageList m_imageList;
	CMenu m_menu;

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
};
