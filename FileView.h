// FileView.h : interface of the CNewzflowView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CNzb;

class CFileView : public CWindowImpl<CFileView, CListViewCtrl>
{
public:
	DECLARE_WND_SUPERCLASS(NULL, CListViewCtrl::GetWndClassName())

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CFileView)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
	END_MSG_MAP()

	CFileView();

	void Init(HWND hwndParent);
	void SetNzb(CNzb* nzb);

	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

protected:
	CNzb* nzb;

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
};
