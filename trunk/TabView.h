#pragma once

class CTabView : public CWindowImpl<::CTabView, CTabCtrl>
{
public:
	DECLARE_WND_SUPERCLASS(NULL, CTabCtrl::GetWndClassName())

	BEGIN_MSG_MAP(CTabView)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		REFLECTED_NOTIFY_CODE_HANDLER(TCN_SELCHANGE, OnSelChange)
		//MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		FORWARD_NOTIFICATIONS()
	END_MSG_MAP()

	int m_nActivePage;

	CTabView() : m_nActivePage(-1)
	{
	}

	int GetPageCount() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return GetItemCount();
	}

	bool IsValidPageIndex(int nPage) const
	{
		return (nPage >= 0 && nPage < GetPageCount());
	}

	bool AddPage(HWND hWndView, LPCTSTR lpstrTitle, int nImage = -1, LPVOID pData = NULL)
	{
		return InsertPage(GetPageCount(), hWndView, lpstrTitle, nImage, pData);
	}

	bool InsertPage(int nPage, HWND hWndView, LPCTSTR lpstrTitle, int nImage = -1, LPVOID pData = NULL)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(nPage == GetPageCount() || IsValidPageIndex(nPage));

		TCITEM tcix = { 0 };
		tcix.mask = TCIF_TEXT | /*TCIF_IMAGE | */TCIF_PARAM;
		tcix.pszText = (LPTSTR)lpstrTitle;
		tcix.iImage = nImage;
		tcix.lParam = (LPARAM)hWndView;
		int nItem = InsertItem(nPage, &tcix);
		if(nItem == -1)
			return false;

		return true;
	}

	void RemovePage(int nPage)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(IsValidPageIndex(nPage));

		SetRedraw(FALSE);

		::ShowWindow(GetPageHWND(nPage), FALSE);

		ATLVERIFY(DeleteItem(nPage) != FALSE);

		if(m_nActivePage == nPage)
		{
			m_nActivePage = -1;

			if(nPage > 0)
			{
				SetActivePage(nPage - 1);
			}
			else if(GetPageCount() > 0)
			{
				SetActivePage(nPage);
			}
			else
			{
				SetRedraw(TRUE);
				Invalidate();
				UpdateWindow();
			}
		}
		else
		{
			nPage = (nPage < m_nActivePage) ? (m_nActivePage - 1) : m_nActivePage;
			m_nActivePage = -1;
			SetActivePage(nPage);
		}

		//pT->OnPageActivated(m_nActivePage);
	}

	LRESULT OnSelChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	{
		SetActivePage(GetCurSel());
		//pT->OnPageActivated(m_nActivePage);

		return 0;
	}

	void SetActivePage(int nPage)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(IsValidPageIndex(nPage));

		SetRedraw(FALSE);

		if(m_nActivePage != -1)
			::ShowWindow(GetPageHWND(m_nActivePage), FALSE);
		m_nActivePage = nPage;
		SetCurSel(m_nActivePage);
		::ShowWindow(GetPageHWND(m_nActivePage), TRUE);

		UpdateLayout();

		SetRedraw(TRUE);
		RedrawWindow(NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}

	HWND GetPageHWND(int nPage) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(IsValidPageIndex(nPage));

		TCITEM tcix = { 0 };
		tcix.mask = TCIF_PARAM;
		GetItem(nPage, &tcix);

		return (HWND)tcix.lParam;
	}

	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		UpdateLayout();
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		RECT rc = {0};
		GetClientRect(&rc);
		FillRect((HDC)wParam, &rc, GetSysColorBrush(COLOR_APPWORKSPACE));
		return 1;
	}

	void UpdateLayout()
	{
		if(m_nActivePage == -1)
			return;

		RECT rect = { 0 };
		GetClientRect(&rect);

		HWND hwnd = GetPageHWND(m_nActivePage);

		AdjustRect(FALSE, &rect);

		// resize client window
		if(hwnd != NULL)
			::SetWindowPos(hwnd, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE);
	}
};
