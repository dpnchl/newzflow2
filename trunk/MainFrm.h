// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "NzbView.h"
#include "ConnectionView.h"
#include "TabView.h"
#include "FileView.h"
#include "Util.h"
#include "DropFileHandler.h"

class CSettingsSheet;

class CMainFrame : public CFrameWindowImpl<CMainFrame>, public CUpdateUI<CMainFrame>, public CDropFilesHandler<CMainFrame>,
		public CMessageFilter, public CIdleHandler
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("NewzflowMainFrame"), IDR_MAINFRAME, 0, COLOR_BTNFACE)

	CMainFrame();
	~CMainFrame();

	CNzbView m_list;
	::CTabViewEx m_tab;
	CConnectionView m_connections;
	CFileView m_files;
	CFont m_font;
	CToolBarImageList m_toolBarImageList;
	CSettingsSheet* m_pSettingsDlg;

	int m_vertSplitY; // user wanted position, from bottom
	int m_vertSplitYReal; // real position

	CPoint m_ptSplit;
	CPoint m_ptMouse;

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();
	void UpdateLayout(BOOL bResizeBars = TRUE);

	BEGIN_UPDATE_UI_MAP(CMainFrame)
		UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_NZB_START, UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_NZB_STOP, UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_NZB_PAUSE, UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_NZB_REMOVE, UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_NZB_MOVE_UP, UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_NZB_MOVE_DOWN, UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
		MESSAGE_HANDLER(s_msgTaskbarButtonCreated, OnTaskbarButtonCreated)
		MESSAGE_HANDLER(WM_COPYDATA, OnCopyData)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_FILE_ADD, OnFileAdd)
		COMMAND_ID_HANDLER(ID_FILE_ADD_URL, OnFileAdd)
		COMMAND_ID_HANDLER(ID_NZB_REMOVE, OnNzbRemove)
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_SETTINGS, OnSettings)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnNzbChanged)
		REFLECT_NOTIFICATIONS() // needed for tab control
		CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
		CHAIN_MSG_MAP(CDropFilesHandler<CMainFrame>)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSetCursor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNzbChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnNzbRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnTaskbarButtonCreated(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	// CDropFilesHandler
	BOOL IsReadyForDrop();
	BOOL HandleDroppedFile(LPCTSTR szBuff);
	void EndDropFiles();

protected:
    CComPtr<ITaskbarList3> m_pTaskbarList;

private:
	static const UINT s_msgTaskbarButtonCreated;
};
