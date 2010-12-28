// TvShowView.h : interface of the CNewzflowView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CNzb;

#include "ListViewEx.h"

class CTvShowView : public CWindowImpl<CTvShowView, CListViewCtrl>, public CDynamicColumns<CTvShowView>, public CCustomDraw<CTvShowView>
{
public:
	DECLARE_WND_SUPERCLASS(_T("CTvShowView"), CListViewCtrl::GetWndClassName())

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CTvShowView)
		REFLECTED_NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDblClick)
		CHAIN_MSG_MAP(CDynamicColumns<CTvShowView>)
		CHAIN_MSG_MAP_ALT(CCustomDraw<CTvShowView>, 1)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	CTvShowView();

	void Init(HWND hwndParent);
	void SetShow(int feedId);

	LRESULT OnDblClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	// CCustomDraw overrides
	DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);

	// CSortableList overrides
	int OnRefresh();
	const ColumnInfo* GetColumnInfoArray();

protected:
	static const ColumnInfo s_columnInfo[];

	int m_showId;

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
};
