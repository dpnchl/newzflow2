// RssView.h : interface of the CNewzflowView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CNzb;

#include "ListViewEx.h"

class CRssView : public CWindowImpl<CRssView, CListViewCtrl>, public CSortableList<CRssView>, public CCustomDraw<CRssView>
{
public:
	DECLARE_WND_SUPERCLASS(_T("CRssView"), CListViewCtrl::GetWndClassName())

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CRssView)
		CHAIN_MSG_MAP(CSortableList<CRssView>)
		CHAIN_MSG_MAP_ALT(CCustomDraw<CRssView>, 1)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	CRssView();

	void Init(HWND hwndParent);

	DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);

	// CSortableList overrides
	int OnRefresh();
	const ColumnInfo* GetColumnInfoArray();

protected:
	static const ColumnInfo s_columnInfo[];

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
};
