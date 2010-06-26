// FileView.h : interface of the CNewzflowView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CNzb;

#include "ListViewEx.h"

class CFileView : public CWindowImpl<CFileView, CListViewCtrl>, public CSortableList<CFileView>, public CCustomDraw<CFileView>
{
public:
	DECLARE_WND_SUPERCLASS(_T("CFileView"), CListViewCtrl::GetWndClassName())

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CFileView)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		CHAIN_MSG_MAP(CSortableList<CFileView>)
		CHAIN_MSG_MAP_ALT(CCustomDraw<CFileView>, 1)
	END_MSG_MAP()

	CFileView();

	void Init(HWND hwndParent);
	void SetNzb(CNzb* nzb);

	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);

	// CSortableList overrides
	int OnRefresh();
	const ColumnInfo* GetColumnInfoArray();

protected:
	CNzb* nzb;
	CTheme m_thmProgress;

	static const ColumnInfo s_columnInfo[];

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
};
