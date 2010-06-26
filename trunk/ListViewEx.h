#pragma once

// mix-in classes for CWindowImpl<T, CListViewCtrl>
// CDynamicColumns<T>: 
//   provides support for storing column state in settings, showing/hiding columns via right-click in header area
//   also preserves state (selected/focus/hot) of items during update
	// Example:
	//
	// class CMyListView : public CWindowImpl<CMyListView, CListViewCtrl>, CDynamicColumns<CMyListView>
	// {
	// 	BEGIN_MSG_MAP()
	// 		...
	// 		CHAIN_MSG_MAP(CDynamicColumns<CMyListView>)
	// 	END_MSG_MAP()
	//
	//  call InitDynamicColumns(listviewName) after Creation of listview. Don't use listview::WM_CREATE
	// 
	//  // return pointer to array of column descriptions
	// 	const ColumnInfo* GetColumnInfoArray()
	// 	{
	// 		return s_columnInfo;
	// 	}
	// 
	//  // called to refresh the list view; list view is empty at this time
	//  // use AddItemEx(), SetItemEx(), SetItemTextEx(), etc. to populate list view
	// 	void OnRefresh()
	// 	{
	//      ...
	// 	}
	// };
	// 
	// /*static*/ const CMyListView::ColumnInfo CMyListView::s_columnInfo[] = { 
	// 	{ _T("nameLong (used for context menu)"), _T("nameShort (used for column header)"), CMyListView::typeString, LVCFMT_LEFT, 400, true },
	// 	...
	//  { NULL },
	// };
// CSortableList<T>
//   extends CDynamicColumn<T> by sorting capabilities
//   make sure to fill in the right type in ColumnInfo for correct sorting. CDynamicColumn<T>::OnSort() is overridden to support sorting during update

// CDynamicColumns<T>
//////////////////////////////////////////////////////////////////////////

template <class T>
class CDynamicColumns
{
public:
	enum EColumnType {
		typeString,
		typeNumber,
		typeSize,
		typeTimeSpan,
	};

	struct ColumnInfo {
		TCHAR* nameLong;
		TCHAR* nameShort;
		EColumnType type;
		int format;
		int width;
		bool visible;
	};

public:
	CDynamicColumns()
	{
		m_columns = NULL;
		m_maxColumns = 0;
		m_lockUpdate = false;
	}
	~CDynamicColumns()
	{
		delete m_columns;
	}

	void InitDynamicColumns(const CString& name)
	{
		T* pT = static_cast<T*>(this);
		pT->SetExtendedListViewStyle(pT->GetExtendedListViewStyle() | LVS_EX_HEADERDRAGDROP);
		const ColumnInfo* columnInfo = pT->GetColumnInfoArray();
		for(m_maxColumns = 0; columnInfo[m_maxColumns].nameLong != NULL; m_maxColumns++) {
			pT->AddColumn(columnInfo[m_maxColumns].nameShort, m_maxColumns, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, columnInfo[m_maxColumns].format);
			pT->SetColumnWidth(m_maxColumns, columnInfo[m_maxColumns].width);
		}

		m_columns = new int[m_maxColumns];
		for(int i = 0; i < m_maxColumns; i++)
			m_columns[i] = i;

		m_name = name;

		CNewzflow::Instance()->settings->GetListViewColumns(m_name, *pT, m_columns, m_maxColumns);
	}

	int SubItemFromColumn(int item)
	{
		return m_columns[item];
	}

	// replacement for CListViewCtrl::AddItem; supports restoring of previous selection/focus state
	int AddItemEx(int nItem, DWORD_PTR data)
	{
		T* pT = static_cast<T*>(this);
		int i = nItem;
		if(nItem >= pT->GetItemCount())
			i = pT->AddItem(nItem, 0, _T(""));
		int state;
		pT->SetItemData(i, data);
		if(m_stateMap.Lookup(data, state))
			pT->SetItemState(i, state, LVIS_FOCUSED | LVIS_SELECTED);
		return i;
	}

	// replacement for CListViewCtrl::SetItem; supports hidden columns
	BOOL SetItemEx(int nItem, int nSubItem, UINT nMask, LPCTSTR lpszItem, int nImage, UINT nState, UINT nStateMask, LPARAM lParam)
	{
		if(m_columns[nSubItem] < 0)
			return TRUE;
		T* pT = static_cast<T*>(this);
		return pT->SetItem(nItem, m_columns[nSubItem], nMask, lpszItem, nImage, nState, nStateMask, lParam);
	}

	// replacement for CListViewCtrl::SetItemText; supports hidden columns
	BOOL SetItemTextEx(int nItem, int nSubItem, LPCTSTR lpszText)
	{
		if(m_columns[nSubItem] < 0)
			return TRUE;
		T* pT = static_cast<T*>(this);
		return pT->SetItemText(nItem, m_columns[nSubItem], lpszText);
	}

	BEGIN_MSG_MAP(CDynamicColumns<T>)
		NOTIFY_CODE_HANDLER(NM_RCLICK, OnHdnRClick)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	END_MSG_MAP()

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		T* pT = static_cast<T*>(this);
		CNewzflow::Instance()->settings->SetListViewColumns(m_name, *pT, m_columns, m_maxColumns);

		return 0;
	}

	// upon right click on header area, show context menu to show/hide columns, and reset layout of columns(+order)
	LRESULT OnHdnRClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	{
		const DWORD pos = GetMessagePos();
		CPoint pt(GET_X_LPARAM(pos), GET_Y_LPARAM(pos));

		T* pT = static_cast<T*>(this);
		const ColumnInfo* columnInfo = pT->GetColumnInfoArray();

		CMenu menu;
		menu.CreatePopupMenu();
		for(int i = 0; i < m_maxColumns; i++) {
			menu.AppendMenu((m_columns[i] >= 0 ? MF_CHECKED : 0) | MF_STRING, i+1, columnInfo[i].nameLong);
		}
		menu.AppendMenu(MF_SEPARATOR);
		menu.AppendMenu(MF_STRING, 10000, _T("Reset"));

		int ret = (int)menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, *pT);
		if(ret == 0) // cancelled
			return 0;

		LockUpdate();
		pT->SetRedraw(FALSE);

		if(ret < m_maxColumns+1) {
			int column = ret - 1;
			if(m_columns[column] >= 0) {
				// hide column
				int col = m_columns[column];
				m_columns[column] = -1;
				for(int i = 0; i < m_maxColumns; i++) {
					if(m_columns[i] >= column)
						m_columns[i]--;
				}
				pT->DeleteColumn(col);
			} else {
				// show column
				for(int i = 0; i < m_maxColumns; i++) {
					if(m_columns[i] >= column)
						m_columns[i]++;
				}
				m_columns[column] = column;
				pT->InsertColumn(column, columnInfo[column].nameShort, columnInfo[column].format, -1);
				pT->SetColumnWidth(column, columnInfo[column].width);
			}
		} else if(ret == 10000) {
			// reset
			int numCols = pT->GetHeader().GetItemCount();
			for(int i = 0; i < numCols; i++)
				pT->DeleteColumn(0);
			for(int i = 0; i < m_maxColumns; i++) {
				m_columns[i] = i;
				pT->AddColumn(columnInfo[i].nameShort, i, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, columnInfo[i].format);
				pT->SetColumnWidth(i, columnInfo[i].width);
			}
		}

		LockUpdate(false);
		pT->SetRedraw(TRUE);
		Refresh();

		return 0;
	}

	// refreshes list view contents;
	// old state is saved, T::OnRefresh is called to populate the list view, then all excess items are deleted (OnRefresh returns new number of items)
	// list view is sorted (optionally when CSortableList<T> is used)
	void Refresh()
	{
		if(!IsLockUpdate()) {
			LockUpdate();
			T* pT = static_cast<T*>(this);
			pT->SetRedraw(FALSE);

			// save state (focus/selection/hot) for list
			int lvCount = pT->GetItemCount();
			for(int i = 0; i < lvCount; i++) {
				m_stateMap[pT->GetItemData(i)] = pT->GetItemState(i, LVIS_FOCUSED | LVIS_SELECTED);
			}
			int hot = pT->GetHotItem();
			// delete all items
			int oldItems = pT->GetItemCount();

			int numItems = pT->OnRefresh();
			while(oldItems > numItems) {
				pT->DeleteItem(numItems);
				oldItems--;
			}

			pT->OnSort();
			m_stateMap.RemoveAll();

			pT->SetHotItem(hot);
			pT->SetRedraw(TRUE);
			LockUpdate(false);
		}
	}

	// prevent list view from being refreshed
	void LockUpdate(bool lock = true)
	{
		m_lockUpdate = lock;
	}
	bool IsLockUpdate()
	{
		return m_lockUpdate;
	}

	// overrideables
	// list view must provide column info array for layout, sorting, etc.
	const ColumnInfo* GetColumnInfoArray()
	{
		ASSERT(FALSE);
		return NULL;
	}

	// called when list view is being refreshed
	// return the number of items in the list view after the refresh
	int OnRefresh()
	{
		return 0;
	}

	// called when list view is to be sorted
	void OnSort()
	{
	}

private:
	int m_maxColumns;
	int* m_columns;
	bool m_lockUpdate;
	CString m_name;
	CAtlMap<DWORD_PTR, int> m_stateMap;

	template <class T>
	friend class CSortableList;
};

// CSortableList<T>
//////////////////////////////////////////////////////////////////////////

template <class T>
class CSortableList : public CDynamicColumns<T>
{
public:
	CSortableList()
	{
		m_sortColumn = -1; // -1 means: don't sort
		m_sortAsc = true;
	}

	~CSortableList()
	{
	}

	struct CompareData
	{
		HWND lv;
		int column;
		bool asc;
		EColumnType type;

		static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
		{
			CompareData* compareData = (CompareData*)lParamSort;
			CListViewCtrl lv(compareData->lv);

			CString s1, s2;
			lv.GetItemText(lParam1, compareData->column, s1);
			lv.GetItemText(lParam2, compareData->column, s2);

			int ret = 0;

			switch(compareData->type) {
			case typeString: {
				ret = s1.CompareNoCase(s2); 
				break;
			}
			case typeNumber: {
				double f1 = _tstof(s1);
				double f2 = _tstof(s2);
				if(f1 < f2) ret = -1;
				else if(f1 == f2) ret = 0;
				else ret = 1;
				break;
			}
			case typeSize: {
				__int64 size1 = Util::ParseSize(s1);
				__int64 size2 = Util::ParseSize(s2);
				if(size1 < size2) ret = -1;
				else if(size1 == size2) ret = 0;
				else ret = 1;
				break;
			}
			case typeTimeSpan: {
				__int64 span1 = Util::ParseTimeSpan(s1);
				__int64 span2 = Util::ParseTimeSpan(s2);
				if(span1 < span2) ret = -1;
				else if(span1 == span2) ret = 0;
				else ret = 1;
				break;
			}
			}

			if(!compareData->asc) ret = -ret;

			return ret;
		}
	};

	void SetSortColumn(int column, bool asc)
	{
		m_sortColumn = column;
		m_sortAsc = asc;
	}

	void OnSort()
	{
		if(m_sortColumn >= 0) {
			T* pT = static_cast<T*>(this);
			CompareData compareData = { *pT, m_sortColumn, m_sortAsc, pT->GetColumnInfoArray()[m_sortColumn].type };
			pT->SortItemsEx(CompareData::CompareFunc, (LPARAM)&compareData);
		}
	}

	BEGIN_MSG_MAP(CSortableList<T>)
		NOTIFY_CODE_HANDLER(HDN_ITEMCLICK, OnHdnItemClick)
		CHAIN_MSG_MAP(CDynamicColumns<T>)
	END_MSG_MAP()

	LRESULT OnHdnItemClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		T* pT = static_cast<T*>(this);
		LPNMHEADER lpnm = (LPNMHEADER)pnmh;

		for(int i = 0; i < pT->GetHeader().GetItemCount(); i++) {
			HDITEM hditem = {0};
			hditem.mask = HDI_FORMAT;
			pT->GetHeader().GetItem(i, &hditem);
			if (i == lpnm->iItem) {
				if(hditem.fmt & HDF_SORTDOWN)
					hditem.fmt = (hditem.fmt & ~HDF_SORTDOWN) | HDF_SORTUP;
				else
					hditem.fmt = (hditem.fmt & ~HDF_SORTUP) | HDF_SORTDOWN;
			} else {
				hditem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
			}
			pT->GetHeader().SetItem(i, &hditem);
		}
		pT->SetSelectedColumn(lpnm->iItem);
		if(m_sortColumn == lpnm->iItem)
			m_sortAsc = !m_sortAsc;
		else {
			m_sortColumn = lpnm->iItem;
			m_sortAsc = true;
		}
		Refresh();
		return 0;
	}

private:
	int m_sortColumn;
	bool m_sortAsc;
};
