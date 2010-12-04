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
		REFLECTED_NOTIFY_CODE_HANDLER(NM_RCLICK, OnRClick)
		COMMAND_ID_HANDLER(ID_FILE_PAUSE, OnFilePause)
		COMMAND_ID_HANDLER(ID_FILE_UNPAUSE, OnFileUnpause)
		CHAIN_MSG_MAP(CSortableList<CFileView>)
		CHAIN_MSG_MAP_ALT(CCustomDraw<CFileView>, 1)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	CFileView();

	void Init(HWND hwndParent);
	void SetNzb(CNzb* nzb);

	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnFilePause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileUnpause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);
	DWORD OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/);

	// CSortableList overrides
	int OnRefresh();
	const ColumnInfo* GetColumnInfoArray();

protected:
	CNzb* nzb;
	CTheme m_thmProgress;
	CMenu m_menu;

	static const ColumnInfo s_columnInfo[];

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
};
