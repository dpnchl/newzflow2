// ViewTree.cpp : implementation of the CViewTree class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "ViewTree.h"
#include "Newzflow.h"
#include "Util.h"
#include "Settings.h"
#include "DialogEx.h"
#include "DropTarget.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

CViewTree::CViewTree()
{
	m_menu.LoadMenu(IDR_TREEVIEW);
}

BOOL CViewTree::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

void CViewTree::Init(HWND hwndParent)
{
	Create(hwndParent, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_TRACKSELECT | TVS_SHOWSELALWAYS | TVS_FULLROWSELECT, WS_EX_CLIENTEDGE);
	SetExtendedStyle(TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
	SetWindowTheme(*this, L"explorer", NULL);

	m_imageList.Create(16, 16, ILC_COLOR32, 0, 100);
	CImage image;
	if(!SUCCEEDED(image.Load(CNewzflow::Instance()->settings->GetAppDataDir() + _T("tstatus.bmp")))) // then try to load µTorrent-compatible images from disk... 
		image.LoadFromResource(ModuleHelper::GetResourceInstance(), IDB_STATUS); // ...or from resource
	m_imageList.Add(image, (HBITMAP)NULL);
	SetImageList(m_imageList, TVSIL_NORMAL);
	SetIndent(8);

	// don't refresh here, because CMainFrame hasn't initialized all of its views yet
}

void CViewTree::Refresh()
{
	int oldSel = kDownloads;
	CTreeItem oldSelItem = GetSelectedItem();
	if(!oldSelItem.IsNull())
		oldSel = oldSelItem.GetData();

	bool oldSelRestored = false;

	SelectItem(NULL); // remove old selection before deleting the items, otherwise we get lots of unwanted selchanged notifications
	DeleteAllItems();
	tvDownloads = InsertItem(_T("Downloads"), 0, 0, TVI_ROOT, TVI_LAST);
	tvFeeds = InsertItem(_T("Feeds"), 13, 13, TVI_ROOT, TVI_LAST);
	tvDownloads.SetData(kDownloads);
	tvFeeds.SetData(kFeeds + 0);

	if(oldSel == tvDownloads.GetData()) {
		tvDownloads.Select();
		oldSelRestored = true;
	}
	if(oldSel == tvFeeds.GetData()) {
		tvFeeds.Select();
		oldSelRestored = true;
	}

	sq3::Statement st(CNewzflow::Instance()->database, _T("SELECT rowid, title FROM RssFeeds ORDER BY title ASC"));
	sq3::Reader reader = st.ExecuteReader();
	while(reader.Step() == SQLITE_ROW) {
		int id; reader.GetInt(0, id);
		CString sTitle; reader.GetString(1, sTitle);
		CTreeItem tvRss = InsertItem(sTitle, 13, 13, tvFeeds, TVI_LAST);
		tvRss.SetData(kFeeds + id);
		if(oldSel == tvRss.GetData()) {
			tvRss.Select();
			oldSelRestored = true;
		}
	}
	tvFeeds.Expand();

	// we couldn't select the same item as before
	if(!oldSelRestored) {
		if(oldSel >= kFeeds)
			tvFeeds.Select();
		else
			tvDownloads.Select();
	}
}

LRESULT CViewTree::OnRClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	DWORD lParam = GetMessagePos();
	CPoint ptScreen(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	CPoint ptClient(ptScreen);
	ScreenToClient(&ptClient);
	UINT flags;
	CTreeItem t = HitTest(ptClient, &flags);
	if(!t.IsNull()) {
		t.Select();
		m_menu.GetSubMenu(0).EnableMenuItem(ID_FEEDS_DELETE, (t.GetData() >= kFeeds+1 ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
		m_menu.GetSubMenu(0).EnableMenuItem(ID_FEEDS_EDIT, (t.GetData() >= kFeeds+1 ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
		m_menu.GetSubMenu(0).TrackPopupMenu(TPM_RIGHTBUTTON, ptScreen.x, ptScreen.y, *this);
	}

	return 0;
}

// non-modal
class CAddFeedDialog : public CDialogImplEx<CAddFeedDialog>, public CWinDataExchangeEx<CAddFeedDialog>
{
public:
	enum { IDD = IDD_ADD_FEED };

	// Construction
	CAddFeedDialog(CViewTree* pViewTree, int iRowId) // when iRowId != 0, create dialog as "Edit Feed", otherwise "Add Feed"
	{
		m_pViewTree = pViewTree;
		m_iRowId = iRowId;
	}
	~CAddFeedDialog()
	{
	}

	// Maps
	BEGIN_MSG_MAP(CAddFeedDialog)
		MSG_WM_INITDIALOG(OnInitDialog)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		COMMAND_HANDLER(IDC_ALIAS_CHECK, BN_CLICKED, OnAliasCheck)
		COMMAND_HANDLER(IDOK, BN_CLICKED, OnOK)
		COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnCancel)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		CHAIN_MSG_MAP(CWinDataExchangeEx<CAddFeedDialog>)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CAddFeedDialog)
		DDX_TEXT(IDC_URL, m_sURL)
		DDX_TEXT(IDC_ALIAS, m_sAlias)
	END_DDX_MAP()

	// Message handlers
	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam)
	{
		CComObject<CDropTarget<CAddFeedDialog> >* pDropTarget;
		CComObject<CDropTarget<CAddFeedDialog> >::CreateInstance(&pDropTarget);
		pDropTarget->Register(*this, this);
		CenterWindow(GetParent());
		if(m_iRowId) {
			sq3::Statement st(CNewzflow::Instance()->database, _T("SELECT title, url FROM RssFeeds WHERE rowid = ?"));
			st.Bind(0, m_iRowId);
			sq3::Reader reader = st.ExecuteReader();
			if(reader.Step() == SQLITE_ROW) {
				reader.GetString(0, m_sAlias);
				reader.GetString(1, m_sURL);
				m_sOldURL = m_sURL;
				if(!m_sAlias.IsEmpty()) {
					CheckDlgButton(IDC_ALIAS_CHECK, 1);
					GetDlgItem(IDC_ALIAS).EnableWindow();
				}

				CString s;
				GetWindowText(s);
				s.Replace(_T("Add"), _T("Edit"));
				SetWindowText(s);

				DoDataExchange(false);
			} else {
				m_iRowId = 0;
			}
		}
		ShowWindow(SW_SHOW);
		return TRUE;
	}
	LRESULT OnAliasCheck(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		GetDlgItem(IDC_ALIAS).EnableWindow(IsDlgButtonChecked(IDC_ALIAS_CHECK));
		return 0;
	}
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		DoDataExchange(true);
		CString s(m_sURL);
		s.MakeLower();
		if(s.Find(_T("http://")) != 0 && s.Find(_T("https://")) != 0) {
			OnDataCustomError(IDC_URL, _T("Please enter a valid http:// or https:// URL."));
		} else {
			if(m_iRowId == 0) {
				sq3::Statement st(CNewzflow::Instance()->database, _T("INSERT OR IGNORE INTO RssFeeds (title, url) VALUES (?, ?)"));
				ASSERT(st.IsValid());
				if(IsDlgButtonChecked(IDC_ALIAS_CHECK))
					st.Bind(0, m_sAlias);
				else
					st.Bind(0);
				st.Bind(1, m_sURL);
				if(st.ExecuteNonQuery() != SQLITE_OK) {
					TRACE(_T("DB error: %s\n"), CNewzflow::Instance()->database.GetErrorMessage());
				}
			} else {
				sq3::Statement st(CNewzflow::Instance()->database, _T("UPDATE RssFeeds SET title = ?, url = ? WHERE rowid = ?"));
				ASSERT(st.IsValid());
				if(IsDlgButtonChecked(IDC_ALIAS_CHECK))
					st.Bind(0, m_sAlias);
				else
					st.Bind(0);
				st.Bind(1, m_sURL);
				st.Bind(2, m_iRowId);
				if(st.ExecuteNonQuery() != SQLITE_OK) {
					TRACE(_T("DB error: %s\n"), CNewzflow::Instance()->database.GetErrorMessage());
				}
				if(m_sURL != m_sOldURL) {
					sq3::Statement st(CNewzflow::Instance()->database, _T("UPDATE RssFeeds SET last_update = NULL WHERE rowid = ?"));
					st.Bind(0, m_iRowId);
					if(st.ExecuteNonQuery() != SQLITE_OK) {
						TRACE(_T("DB error: %s\n"), CNewzflow::Instance()->database.GetErrorMessage());
					}
				}
			}
			DestroyWindow();
			m_pViewTree->Refresh();
			CNewzflow::Instance()->RefreshRssWatcher();
		}
		return 0;
	}
	void Cancel()
	{
		DestroyWindow();
	}
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		Cancel();
		return 0;
	}
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		Cancel();
		return 0;
	}
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		::RevokeDragDrop(*this);
		return 0;
	}

	// CDropTarget
	void OnDropFile(LPCTSTR szFile)
	{
	}
	void OnDropURL(LPCTSTR szURL)
	{
		m_sURL = szURL;
		DoDataExchange(false);
	}

	virtual void OnFinalMessage(HWND)
	{
		delete this;
	}

	// DDX variables
	CString m_sOldURL;
	CString m_sURL, m_sAlias;
	CViewTree* m_pViewTree;
	int m_iRowId;
};

LRESULT CViewTree::OnFeedsAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAddFeedDialog* dlg = new CAddFeedDialog(this, 0);
	dlg->Create(*this);

	return 0;
}

LRESULT CViewTree::OnFeedsEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CTreeItem item = GetSelectedItem();
	if(!item.IsNull() && item.GetParent() == tvFeeds) {
		ASSERT(item.GetData() >= kFeeds);
		int feedId = item.GetData() - kFeeds;

		CAddFeedDialog* dlg = new CAddFeedDialog(this, feedId);
		dlg->Create(*this);
	}

	return 0;
}

LRESULT CViewTree::OnFeedsDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CTreeItem item = GetSelectedItem();
	if(!item.IsNull() && item.GetParent() == tvFeeds) {
		ASSERT(item.GetData() >= kFeeds);
		int feedId = item.GetData() - kFeeds;
		{ sq3::Transaction transaction(CNewzflow::Instance()->database);
			{
				sq3::Statement st(CNewzflow::Instance()->database, _T("DELETE FROM RssItems WHERE feed = ?"));
				ASSERT(st.IsValid());
				st.Bind(0, feedId);
				if(st.ExecuteNonQuery() != SQLITE_OK) {
					TRACE(_T("DB error: %s\n"), CNewzflow::Instance()->database.GetErrorMessage());
				}
			}

			{
				sq3::Statement st(CNewzflow::Instance()->database, _T("DELETE FROM RssFeeds WHERE rowid = ?"));
				ASSERT(st.IsValid());
				st.Bind(0, feedId);
				if(st.ExecuteNonQuery() != SQLITE_OK) {
					TRACE(_T("DB error: %s\n"), CNewzflow::Instance()->database.GetErrorMessage());
				}
			}
			transaction.Commit();
		}

		Refresh();
	}

	return 0;
}
