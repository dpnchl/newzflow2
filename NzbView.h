// NewzflowView.h : interface of the CNewzflowView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CNzbView : public CWindowImpl<CNzbView, CListViewCtrl>, public CCustomDraw<CNzbView>
{
public:
	DECLARE_WND_SUPERCLASS(NULL, CListViewCtrl::GetWndClassName())

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CNzbView)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		NOTIFY_CODE_HANDLER(NM_RCLICK, OnHdnRClick)
		NOTIFY_CODE_HANDLER(HDN_ITEMCLICK, OnHdnItemClick)
		CHAIN_MSG_MAP_ALT(CCustomDraw<CNzbView>, 1)
	END_MSG_MAP()

	CNzbView();
	~CNzbView();

	void Init(HWND hwndParent);

	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnHdnItemClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnHdnRClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);

	// these methods can handle hidden columns
	BOOL SetItemEx(int nItem, int nSubItem, UINT nMask, LPCTSTR lpszItem, int nImage, UINT nState, UINT nStateMask, LPARAM lParam);
	BOOL SetItemTextEx(int nItem, int nSubItem, LPCTSTR lpszText);

public:
	enum EColumnType {
		typeString,
		typeNumber,
		typeSize,
		typeTimeSpan,
	};

	struct ColumnInfo {
		TCHAR* nameLong;
		TCHAR* nameShort;
		EColumnType type;
		int format;
		int width;
		bool visible;
	};

protected:
	CTheme m_thmProgress;
	CImageList m_imageList;


	static const ColumnInfo s_columnInfo[];
	int* m_columns;
	int m_sortColumn;
	bool m_sortAsc;
	bool m_lockUpdate;


// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
};
