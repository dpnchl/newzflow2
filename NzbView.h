// NewzflowView.h : interface of the CNewzflowView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ListViewEx.h"

class CNzbView : public CWindowImpl<CNzbView, CListViewCtrl>, public CDynamicColumns<CNzbView>,  public CCustomDraw<CNzbView>
{
public:
	DECLARE_WND_SUPERCLASS(_T("CNzbView"), CListViewCtrl::GetWndClassName())

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CNzbView)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		REFLECTED_NOTIFY_CODE_HANDLER(NM_RCLICK, OnRClick)
		COMMAND_ID_HANDLER(ID_DEBUG_POSTPROCESS, OnDebugPostprocess)
		CHAIN_MSG_MAP(CDynamicColumns<CNzbView>)
		CHAIN_MSG_MAP_ALT(CCustomDraw<CNzbView>, 1)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	CNzbView();
	~CNzbView();

	void Init(HWND hwndParent);

	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnDebugPostprocess(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);

	// CDynamicColumns/CSortableList overrides
	int OnRefresh();
	const ColumnInfo* GetColumnInfoArray();

protected:
	CTheme m_thmProgress;
	CImageList m_imageList;
	CMenu m_menu;

	static const ColumnInfo s_columnInfo[];

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
};
