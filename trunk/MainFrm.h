// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ViewTree.h"
#include "NzbView.h"
#include "RssView.h"
#include "ConnectionView.h"
#include "TabView.h"
#include "LogView.h"
#include "FileView.h"
#include "Util.h"
#include "NTray.h"

class CNewzflowStatusBarCtrl : public CMultiPaneStatusBarCtrl
{
public:
	BEGIN_MSG_MAP(CConnectionView)
		MESSAGE_HANDLER(WM_RBUTTONUP, OnRButtonUp)
		CHAIN_MSG_MAP(CMultiPaneStatusBarCtrl)
	END_MSG_MAP()

	LRESULT OnRButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	void OnRClickPause(const CPoint &ptScreen);
	void OnRClickShutdown(const CPoint &ptScreen);
	void OnRClickSpeedLimit(const CPoint &ptScreen);
	void OnRClickConnections(const CPoint &ptScreen);
};

class CMainFrame : public CFrameWindowImpl<CMainFrame>, public CUpdateUI<CMainFrame>, public CMessageFilter, public CIdleHandler
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("NewzflowMainFrame"), IDR_MAINFRAME, 0, COLOR_BTNFACE)

	CMainFrame();
	~CMainFrame();

	CWindow* m_pTopView;
	CViewTree m_TreeView;
	CNzbView m_NzbView;
	CRssView m_RssView;
	CTabViewEx m_TabView[2];
	CConnectionView m_ConnectionView;
	CFileView m_FileView;
	CLogView m_LogView;
	CFont m_font;
	CToolBarImageList m_ToolBarImageList;
	CTrayNotifyIcon m_TrayIcon;

	int m_vertSplitY; // user wanted position, from bottom
	int m_vertSplitYReal; // real position
	int m_horzSplitX;

	CPoint m_ptSplit;
	CPoint m_ptMouse;
	int m_iDragMode; // 0=horz split; 1=vert split

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
		UPDATE_ELEMENT(1, UPDUI_STATUSBAR)
		UPDATE_ELEMENT(2, UPDUI_STATUSBAR)
		UPDATE_ELEMENT(3, UPDUI_STATUSBAR)
		UPDATE_ELEMENT(4, UPDUI_STATUSBAR)
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
		COMMAND_ID_HANDLER(ID_FILE_ADD_URL, OnFileAddUrl)
		COMMAND_ID_HANDLER(ID_NZB_MOVE_UP, OnNzbMoveUp)
		COMMAND_ID_HANDLER(ID_NZB_MOVE_DOWN, OnNzbMoveDown)
		COMMAND_ID_HANDLER(ID_NZB_REMOVE, OnNzbRemove)
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_SETTINGS, OnSettings)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		MESSAGE_HANDLER(Util::MSG_SAVE_NZB, OnSaveNzb)
		MESSAGE_HANDLER(Util::MSG_NZB_FINISHED, OnNzbFinished)
		MESSAGE_HANDLER(Util::MSG_TRAY_NOTIFY, OnTrayNotify)
		MESSAGE_HANDLER(Util::MSG_RSSFEED_UPDATED, OnRssFeedUpdated)
		NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnNzbChanged)
		NOTIFY_CODE_HANDLER(TVN_SELCHANGED, OnTreeChanged)
		CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
		REFLECT_NOTIFICATIONS() // needed for tab control; put below CHAIN_MSG_MAP(CFrameWindowImpl), otherwise tooltips don't work
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
	LRESULT OnTreeChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	void UpdateNzbButtons();
	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileAddUrl(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSaveNzb(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnNzbFinished(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRssFeedUpdated(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTrayNotify(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnNzbRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnNzbMoveUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnNzbMoveDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnTaskbarButtonCreated(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	// CDropTarget
	void OnDropFile(LPCTSTR szBuff);
	void OnDropURL(LPCTSTR szBuff);

protected:
    CComPtr<ITaskbarList3> m_pTaskbarList;
	CNewzflowStatusBarCtrl m_statusBar;

private:
	static const UINT s_msgTaskbarButtonCreated;
};
