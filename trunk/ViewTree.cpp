// ViewTree.cpp : implementation of the CViewTree class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "ViewTree.h"
#include "Newzflow.h"
#include "Util.h"
#include "Settings.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

CViewTree::CViewTree()
{
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

	Refresh();
}

void CViewTree::Refresh()
{
	tvDownloads = InsertItem(_T("Downloads"), 0, 0, TVI_ROOT, TVI_LAST);
	tvFeeds = InsertItem(_T("Feeds"), 13, 13, TVI_ROOT, TVI_LAST);
	SetItemData(tvFeeds, 0);

	sq3::Statement st(CNewzflow::Instance()->database, _T("SELECT rowid, name FROM RssFeeds ORDER BY name ASC"));
	sq3::Reader reader = st.ExecuteReader();
	while(reader.Step() == SQLITE_ROW) {
		int id; reader.GetInt(0, id);
		CString sName; reader.GetString(1, sName);
		CTreeItem tvRss = InsertItem(sName, 13, 13, tvFeeds, TVI_LAST);
		SetItemData(tvRss, id);
	}
	tvFeeds.Expand();
}