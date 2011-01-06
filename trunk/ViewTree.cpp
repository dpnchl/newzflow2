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
#include "TheTvDB.h"
#include "Database.h"

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
	tvDownloads = InsertItem(_T("Downloads"), 0, 0, TVI_ROOT, TVI_LAST); tvDownloads.SetData(kDownloads);
	tvFeeds = InsertItem(_T("Feeds"), 13, 13, TVI_ROOT, TVI_LAST); tvFeeds.SetData(kFeeds + 0);
	tvTvShows = InsertItem(_T("TV Shows"), 12, 12, TVI_ROOT, TVI_LAST); tvTvShows.SetData(kTvShows + 0);
	tvMovies = InsertItem(_T("Movies"), 12, 12, TVI_ROOT, TVI_LAST); tvMovies.SetData(kMovies);

	if(oldSel == tvDownloads.GetData()) {
		tvDownloads.Select();
		oldSelRestored = true;
	}
	if(oldSel == tvFeeds.GetData()) {
		tvFeeds.Select();
		oldSelRestored = true;
	}
	if(oldSel == tvMovies.GetData()) {
		tvMovies.Select();
		oldSelRestored = true;
	}

	// add RSS feeds
	QRssFeeds* rssFeeds = CNewzflow::Instance()->database->GetRssFeeds();
	while(rssFeeds->GetRow()) {
		CTreeItem tvRss = InsertItem(rssFeeds->GetTitle(), 13, 13, tvFeeds, TVI_LAST);
		tvRss.SetData(kFeeds + rssFeeds->GetId());
		if(oldSel == tvRss.GetData()) {
			tvRss.Select();
			oldSelRestored = true;
		}
	}
	tvFeeds.Expand();
	delete rssFeeds;

	// add TV shows
	QTvShows* tvShows = CNewzflow::Instance()->database->GetTvShows();
	while(tvShows->GetRow()) {
		CTreeItem tvShow = InsertItem(tvShows->GetTitle(), 12, 12, tvTvShows, TVI_LAST);
		tvShow.SetData(kTvShows + tvShows->GetId());
		if(oldSel == tvShow.GetData()) {
			tvShow.Select();
			oldSelRestored = true;
		}
	}
	tvTvShows.Expand();
	delete tvShows;

	// we couldn't select the same item as before
	if(!oldSelRestored) {
		if(oldSel >= kFeeds && oldSel <= kFeedsEnd)
			tvFeeds.Select();
		else if(oldSel >= kTvShows && oldSel <= kTvShowsEnd)
			tvTvShows.Select();
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
		bool isFeed = t.GetData() >= kFeeds+1 && t.GetData() <= kFeedsEnd;
		bool isTv = t.GetData() >= kTvShows+1 && t.GetData() <= kTvShowsEnd;
		m_menu.GetSubMenu(0).EnableMenuItem(ID_FEEDS_DELETE, (isFeed ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
		m_menu.GetSubMenu(0).EnableMenuItem(ID_FEEDS_EDIT, (isFeed ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
		m_menu.GetSubMenu(0).EnableMenuItem(ID_TV_DELETE, (isTv ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
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
			QRssFeeds* rssFeeds = CNewzflow::Instance()->database->GetRssFeeds(m_iRowId);
			if(rssFeeds->GetRow()) {
				m_sAlias = rssFeeds->GetTitle();
				m_sURL = rssFeeds->GetUrl();
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
			delete rssFeeds;
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
				CNewzflow::Instance()->database->InsertRssFeed(0, IsDlgButtonChecked(IDC_ALIAS_CHECK) ? m_sAlias : _T(""), m_sURL);
			} else {
				CNewzflow::Instance()->database->InsertRssFeed(m_iRowId, IsDlgButtonChecked(IDC_ALIAS_CHECK) ? m_sAlias : _T(""), m_sURL, m_sURL != m_sOldURL);
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
		ASSERT(item.GetData() >= kFeeds + 1 && item.GetData() <= kFeedsEnd);
		int feedId = item.GetData() - kFeeds;
		{ CTransaction transaction(CNewzflow::Instance()->database);
			CNewzflow::Instance()->database->DeleteRssFeed(feedId);
		}
		Refresh();
	}

	return 0;
}

// non-modal
class CAddTvShowDialog : public CDialogImplEx<CAddTvShowDialog>, public CWinDataExchangeEx<CAddTvShowDialog>
{
public:
	enum { IDD = IDD_ADD_TVSHOW };

	// Construction
	CAddTvShowDialog(CViewTree* pViewTree)
	{
		m_pViewTree = pViewTree;
		m_pGetSeries = NULL;
	}
	~CAddTvShowDialog()
	{
		delete m_pGetSeries;
	}

	enum {
		MSG_IMAGELOADED = WM_USER+1,
	};

	// Maps
	BEGIN_MSG_MAP(CAddTvShowDialog)
		MSG_WM_INITDIALOG(OnInitDialog)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(MSG_IMAGELOADED, OnImageLoaded)
		COMMAND_HANDLER(IDOK, BN_CLICKED, OnOK)
		COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnCancel)
		COMMAND_HANDLER(IDC_SEARCH, BN_CLICKED, OnSearch)
		NOTIFY_HANDLER(IDC_SHOWLIST, LVN_ITEMCHANGED, OnItemChanged)
		NOTIFY_HANDLER(IDC_SHOWLIST, NM_DBLCLK, OnDblclk)
		CHAIN_MSG_MAP(CWinDataExchangeEx<CAddTvShowDialog>)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CAddTvShowDialog)
		DDX_TEXT(IDC_SHOWNAME, m_sShowName)
	END_DDX_MAP()

	// capture RETURN key when IDC_SHOWNAME has focus to simulate a click on IDC_SEARCH
	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		if(GetFocus() == GetDlgItem(IDC_SHOWNAME) && pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
			BOOL b;
			OnSearch(BN_CLICKED, IDC_SEARCH, GetDlgItem(IDC_SEARCH), b);
			return TRUE;
		}
		return CDialogImplEx<CAddTvShowDialog>::PreTranslateMessage(pMsg);
	}

	// Message handlers
	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam)
	{
		CenterWindow(GetParent());
		ShowWindow(SW_SHOW);
		GetDlgItem(IDOK).EnableWindow(FALSE);
		m_List = GetDlgItem(IDC_SHOWLIST);
		m_List.SetView(LV_VIEW_TILE);
		m_List.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_AUTOSIZECOLUMNS);
		m_List.AddColumn(_T("Show"), 0);
		m_List.AddColumn(_T("First Aired"), 1);
		m_List.AddColumn(_T("Overview"), 2);
		LVTILEVIEWINFO tvi = { 0 };
		tvi.cbSize = sizeof(tvi);
		tvi.dwFlags = LVTVIF_FIXEDSIZE;
		tvi.dwMask = LVTVIM_TILESIZE | LVTVIM_COLUMNS;
		CRect r;
		m_List.GetClientRect(&r);
		tvi.cLines = 3;
		SIZE size = { r.Width() - ::GetSystemMetrics(SM_CXVSCROLL), 60 };
		tvi.sizeTile = size;
		m_List.SetTileViewInfo(&tvi);
		GetDlgItem(IDOK).ModifyStyle(BS_DEFPUSHBUTTON, 0);
		GetDlgItem(IDC_SEARCH).ModifyStyle(0, BS_DEFPUSHBUTTON);
		SetWindowTheme(m_List, L"explorer", NULL);
		m_ImageLoader.SetSink(*this, MSG_IMAGELOADED);
		m_ImageLoader.Init(4);
		m_ImageList.Create(300, 55, true);

		return TRUE;
	}
	LRESULT OnSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		DoDataExchange(true);
		if(m_sShowName.IsEmpty()) {
			OnDataCustomError(IDC_SHOWNAME, _T("Please enter a show name."));
			return 0;
		}
		CWaitCursor wait;
		delete m_pGetSeries;
		m_pGetSeries = CNewzflow::Instance()->tvdbApi->GetSeries(m_sShowName);
		if(!m_pGetSeries) {
			OnDataCustomError(IDC_SEARCH, _T("Error connecting to TheTVDB.com"));
			return 0;
		}
		m_List.DeleteAllItems();
		CHttpDownloader downloader;
		downloader.Init();
		m_ImageLoader.Clear();
		m_ImageList.RemoveAll();
		m_ImageList.AddEmpty();
		m_ImageList.AddIcon(LoadIcon(LoadLibrary(_T("shell32.dll")), MAKEINTRESOURCE(1004)), RGB(0xf0, 0xf0, 0xf0) | 0xff000000, RGB(0xd0, 0xd0, 0xd0) | 0xff000000);

		for(size_t i = 0; i < m_pGetSeries->Series.GetCount(); i++) {
			TheTvDB::CSeries* series = m_pGetSeries->Series[i];
			int id = m_List.InsertItem(i, series->SeriesName, 0);
			if(series->FirstAired.m_dt != 0)
				m_List.SetItemText(id, 1, series->FirstAired.Format(_T("First Aired: %Y-%m-%d")));
			m_List.SetItemText(id, 2, series->Overview);
			m_List.SetItemData(id, i);
			LVTILEINFO ti = { 0 };
			UINT columns[] = { 2, 1 };
			ti.cbSize = sizeof(ti) - sizeof(ti.piColFmt); // for XP compatibility
			ti.iItem = id;
			ti.cColumns = 2;
			ti.puColumns = columns;
			m_List.SetTileInfo(&ti);
			if(!series->banner.IsEmpty()) {
				CString bannerFile;
				bannerFile.Format(_T("%sseries%d.jpg"), CNewzflow::Instance()->settings->GetAppDataDir(), series->id);
				int imageId = m_ImageList.Add(bannerFile);
				if(imageId >= 0) {
					m_List.SetItem(id, 0, LVIF_IMAGE, NULL, imageId, 0, 0, 0);
				} else {
					m_ImageLoader.Add(_T("http://www.thetvdb.com/banners/") + series->banner, bannerFile, id);
				}
			} else {
				m_List.SetItem(id, 0, LVIF_IMAGE, NULL, 1, 0, 0, 0); // "no image"
			}
		}
		m_List.SetImageList(m_ImageList, LVSIL_NORMAL);
		return 0;
	}
	LRESULT OnImageLoaded(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CAsyncDownloader::CItem* item = (CAsyncDownloader::CItem*)wParam;
		int image = m_ImageList.Add(item->path);
		if(image >= 0)
			m_List.SetItem(item->param, 0, LVIF_IMAGE, NULL, image, 0, 0, 0);
		return 0;
	}
	LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	{
		GetDlgItem(IDOK).EnableWindow(m_List.GetSelectedCount() > 0);
		return 0;
	}
	LRESULT OnDblclk(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
	{
		if(m_List.GetSelectedCount() > 0)
			return OnOK(0, 0, NULL, bHandled);
		return 0;
	}
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		DoDataExchange(true);

		int iSelection = m_List.GetSelectedIndex();
		ASSERT(iSelection >= 0);

		TheTvDB::CSeries* series = m_pGetSeries->Series[m_List.GetItemData(iSelection)];

		CNewzflow::Instance()->database->InsertTvShow(series);

		DestroyWindow();
		m_pViewTree->Refresh();
		CNewzflow::Instance()->RefreshRssWatcher();
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
	virtual void OnFinalMessage(HWND)
	{
		delete this;
	}

	// DDX variables
	CString m_sShowName;

	CViewTree* m_pViewTree;
	CListViewCtrl m_List;
	CImageListEx m_ImageList;

	TheTvDB::CGetSeries* m_pGetSeries;

	CAsyncDownloader m_ImageLoader;
};

LRESULT CViewTree::OnTvAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAddTvShowDialog* dlg = new CAddTvShowDialog(this);
	dlg->Create(*this);

	return 0;
}

LRESULT CViewTree::OnTvDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CTreeItem item = GetSelectedItem();
	if(!item.IsNull() && item.GetParent() == tvTvShows) {
		ASSERT(item.GetData() >= kTvShows + 1 && item.GetData() <= kTvShowsEnd);
		int showId = item.GetData() - kTvShows;
		CNewzflow::Instance()->database->DeleteTvShow(showId);

		Refresh();
	}

	return 0;
}