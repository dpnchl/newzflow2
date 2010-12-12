// LogView.h : interface of the CNewzflowView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CNzb;

class CLogView : public CWindowImpl<CLogView, CRichEditCtrl>, public CRichEditCommands<CLogView>
{
public:
	DECLARE_WND_SUPERCLASS(_T("CLogView"), CRichEditCtrl::GetWndClassName())

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CLogView)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		//REFLECTED_NOTIFY_CODE_HANDLER(NM_RCLICK, OnRClick)
		//REFLECT_NOTIFICATIONS()
		CHAIN_MSG_MAP_ALT(CRichEditCommands<CLogView>, 1)
	END_MSG_MAP()

	CLogView();

	void Init(HWND hwndParent);
	void SetNzb(CNzb* nzb);

	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	void Refresh();

protected:
	CNzb* nzb;
	CString oldLog;

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
};
