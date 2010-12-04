#pragma once

// use CPropertyPageImplEx instead of CPropertyPageImpl; CPropertyPageEx instead of CPropertyPage
// and CPropertySheetImplEx instead of CPropertySheetImpl (also adds support for keyboard navigation in modeless property sheets)
// use CDialogImplEx instead of CDialogImpl (also adds support for keyboard navigation in modeless dialogs)
// to automatically replace the property sheet's, property pages' and dialog's font with NONCLIENTMETRICS.MessageFont (Segoe UI on Vista/Win7)

// use CWinDataExchangeEx instead of CWinDataExchange to enable balloon tips for data validation
// use CHAIN_MSG_MAP(CWinDataExchangeEx<T>) to forward necessary messages

#include "DialogTemplate.h"

namespace {
	// Idea from http://www.codeguru.com/cpp/controls/propertysheet/previoussectionmanager/article.php/c16651/Custom-Font-in-Property-Sheets.htm
	static inline void SetMessageFontDlgTemplate(DLGTEMPLATE* pResource, HWND hWnd)
	{
		NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
		#if(WINVER >= 0x0600)
			OSVERSIONINFO osvi;
			ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
			osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			GetVersionEx(&osvi);
			if(osvi.dwMajorVersion < 6) ncm.cbSize -= sizeof(ncm.iPaddedBorderWidth);
		#endif
		if(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, false)) {
			CDialogTemplate dlgTemplate(pResource);
			HDC dc = ::GetDC(NULL);
			int points = -int(ncm.lfMessageFont.lfHeight * 72 / GetDeviceCaps(dc, LOGPIXELSY));
			::ReleaseDC(NULL, dc);
			dlgTemplate.SetFont(ncm.lfMessageFont.lfFaceName, (WORD)points);
			memmove((void*)pResource, dlgTemplate.m_hTemplate, dlgTemplate.m_dwTemplateSize);
		}
	}
}

template <class T, class TBase = CWindow>
class ATL_NO_VTABLE CDialogImplEx : public CDialogImplBaseT<TBase>, CMessageFilter
{
public:
	~CDialogImplEx()
	{
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->RemoveMessageFilter(this);
	}

	// modal dialogs
	INT_PTR DoModal(HWND hWndParent = ::GetActiveWindow(), LPARAM dwInitParam = NULL)
	{
		BOOL result;

		ATLASSUME(m_hWnd == NULL);

		// Allocate the thunk structure here, where we can fail
		// gracefully.

		result = m_thunk.Init(NULL,NULL);
		if (result == FALSE) 
		{
			SetLastError(ERROR_OUTOFMEMORY);
			return -1;
		}

		DLGTEMPLATE* pDlgTemplate = GetDlgTemplate(hWndParent);
		if(!pDlgTemplate)
			return -1;

		_AtlWinModule.AddCreateWndData(&m_thunk.cd, (CDialogImplBaseT<TBase>*)this);
		INT_PTR ret = ::DialogBoxIndirectParam(_AtlBaseModule.GetResourceInstance(), pDlgTemplate, hWndParent, T::StartDialogProc, dwInitParam);
		delete (char*)pDlgTemplate;
		return ret;
	}
	BOOL EndDialog(int nRetCode)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::EndDialog(m_hWnd, nRetCode);
	}
	// modeless dialogs
	HWND Create(HWND hWndParent, LPARAM dwInitParam = NULL)
	{
		BOOL result;

		ATLASSUME(m_hWnd == NULL);

		// Allocate the thunk structure here, where we can fail
		// gracefully.

		result = m_thunk.Init(NULL,NULL);
		if (result == FALSE) 
		{
			SetLastError(ERROR_OUTOFMEMORY);
			return NULL;
		}

		DLGTEMPLATE* pDlgTemplate = GetDlgTemplate(hWndParent);
		if(!pDlgTemplate)
			return NULL;

		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);

		_AtlWinModule.AddCreateWndData(&m_thunk.cd, (CDialogImplBaseT< TBase >*)this);
		HWND hWnd = ::CreateDialogIndirectParam(_AtlBaseModule.GetResourceInstance(), pDlgTemplate, hWndParent, T::StartDialogProc, dwInitParam);
		delete (char*)pDlgTemplate;
		ATLASSUME(m_hWnd == hWnd);
		return hWnd;
	}
	// for CComControl
	HWND Create(HWND hWndParent, RECT&, LPARAM dwInitParam = NULL)
	{
		return Create(hWndParent, dwInitParam);
	}
	BOOL DestroyWindow()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::DestroyWindow(m_hWnd);
	}
	BOOL PreTranslateMessage(MSG* pMsg)
	{
		if(pMsg && ((m_hWnd == pMsg->hwnd) || ::IsChild(m_hWnd, pMsg->hwnd))) {
			// Only check keyboard message
			if(pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST) {
				return IsDialogMessage(pMsg);
			}
		}
		return FALSE;
	}
private:
	DLGTEMPLATE* GetDlgTemplate(HWND hWndParent)
	{
		// load dialog template from resource
		HINSTANCE hInstance = ModuleHelper::GetResourceInstance();
		LPCTSTR lpTemplateName = MAKEINTRESOURCE(static_cast<T*>(this)->IDD);
		HRSRC hDlg = ::FindResource(hInstance, lpTemplateName, (LPTSTR)RT_DIALOG);
		if(hDlg != NULL) {
			HGLOBAL hDlgRes = ::LoadResource(hInstance, hDlg);
			DLGTEMPLATE* pDlg = (DLGTEMPLATE*)::LockResource(hDlgRes);
			// make a copy of the resource, can't modify it inplace
			int resSize = ::SizeofResource(hInstance, hDlg);
			DLGTEMPLATE* pDlgTemplate = pDlgTemplate = (DLGTEMPLATE*)new char [resSize];
			memcpy(pDlgTemplate, pDlg, resSize);
			UnlockResource(hDlgRes);
			FreeResource(hDlgRes);
			// patch font
			SetMessageFontDlgTemplate(pDlgTemplate, hWndParent);
			return pDlgTemplate;
		} else {
			return NULL;
		}
	}
};


template <class T, class TBase = CPropertyPageWindow>
class CPropertyPageImplEx : public CPropertyPageImpl<T, TBase>
{
public:
// Data members
	void* m_resource;

	CPropertyPageImplEx(ATL::_U_STRINGorID title = (LPCTSTR)NULL) : 
		CPropertyPageImpl< T, TBase >(title), m_resource(NULL)
	{
		// copy from CAxPropertyPageImpl
		T* pT = static_cast<T*>(this);
		pT;   // avoid level 4 warning

		HINSTANCE hInstance = ModuleHelper::GetResourceInstance();
		LPCTSTR lpTemplateName = MAKEINTRESOURCE(pT->IDD);
		HRSRC hDlg = ::FindResource(hInstance, lpTemplateName, (LPTSTR)RT_DIALOG);
		if(hDlg != NULL) {
			HGLOBAL hDlgRes = ::LoadResource(hInstance, hDlg);
			DLGTEMPLATE* pDlg = (DLGTEMPLATE*)::LockResource(hDlgRes);
			// make a copy of the resource, can't modify it inplace
			int resSize = ::SizeofResource(hInstance, hDlg);
			m_resource = new char [resSize];
			memcpy(m_resource, pDlg, resSize);
			UnlockResource(hDlgRes);
			FreeResource(hDlgRes);

			// set up property page to use in-memory dialog template
			m_psp.dwFlags |= PSP_DLGINDIRECT;
			m_psp.pResource = (PROPSHEETPAGE_RESOURCE)m_resource;

			SetMessageFontDlgTemplate((DLGTEMPLATE*)m_psp.pResource, NULL);
		} else {
			ATLASSERT(FALSE && _T("CPropertyPageEx - Cannot find dialog template!"));
		}
	}

	~CPropertyPageImplEx()
	{
		if(m_resource != NULL) {
			delete (char*)m_resource;
		}
	}
};

template <WORD t_wDlgTemplateID>
class CPropertyPageEx : public CPropertyPageImplEx<CPropertyPageEx<t_wDlgTemplateID> >
{
public:
	enum { IDD = t_wDlgTemplateID };

	CPropertyPageEx(ATL::_U_STRINGorID title = (LPCTSTR)NULL) : CPropertyPageImplEx<CPropertyPageEx>(title)
	{ }

	DECLARE_EMPTY_MSG_MAP()
};

// CMessageFilter/PreTranslateMessage necessary for keyboard navigation to work when used as a modeless property sheet
// see http://osdir.com/ml/windows.wtl/2002-11/msg00156.html
template <class T, class TBase = CPropertySheetWindow>
class CPropertySheetImplEx : public CPropertySheetImpl<T, TBase>, public CMessageFilter
{
public:
	CPropertySheetImplEx(ATL::_U_STRINGorID title = (LPCTSTR)NULL, UINT uStartPage = 0, HWND hWndParent = NULL)
		: CPropertySheetImpl<T, TBase>(title, uStartPage, hWndParent) 
	{
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);
	}

	~CPropertySheetImplEx() 
	{
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->RemoveMessageFilter(this);
	}

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		if(pMsg && ((m_hWnd == pMsg->hwnd) || ::IsChild(m_hWnd, pMsg->hwnd))) {
			// Only check keyboard message
			if(pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST) {
				return IsDialogMessage(pMsg);
			}
		}
		return FALSE;
	}

	static int CALLBACK PropSheetCallback(HWND hWnd, UINT uMsg, LPARAM lParam)
	{
		if(uMsg == PSCB_PRECREATE) {
			SetMessageFontDlgTemplate((DLGTEMPLATE*)lParam, hWnd);
		}

		return CPropertySheetImpl<T, TBase>::PropSheetCallback(hWnd, uMsg, lParam);
	}
};

template <class T>
class CWinDataExchangeEx : public CWinDataExchange<T>
{
public:
	BEGIN_MSG_MAP(CWinDataExchangeEx<T>)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
		COMMAND_CODE_HANDLER(EN_KILLFOCUS, OnEnKillFocus);
		COMMAND_CODE_HANDLER(CBN_EDITCHANGE, OnChange)
		COMMAND_CODE_HANDLER(CBN_KILLFOCUS, OnEnKillFocus);
		MESSAGE_RANGE_HANDLER(WM_MOUSEFIRST, WM_MOUSELAST, OnMouseMessage)
	END_MSG_MAP()

	LRESULT OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
		if(m_tip.IsWindow())
			m_tip.TrackActivate(&m_ti, FALSE);
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnEnKillFocus(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
	{
		if(m_tip.IsWindow())
			m_tip.TrackActivate(&m_ti, FALSE);
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		T* pT = static_cast<T*>(this);
		if(m_tip.IsWindow())
			m_tip.RelayEvent((LPMSG)pT->GetCurrentMessage());
		bHandled = FALSE;
		return 0;
	}

	// override from CWinDataExchange
	void OnDataValidateError(UINT nCtrlID, BOOL bSave, _XData& data)
	{
		switch(data.nDataType) {
		case ddxDataInt: m_tiText.Format(_T("Please enter a number between %d and %d."), data.intData.nMin, data.intData.nMax); break;
		case ddxDataFloat: case ddxDataDouble: m_tiText.Format(_T("Please enter a number between %g and %g."), data.floatData.nMin, data.floatData.nMax); break;
		default: m_tiText.Format(_T("Please enter a valid value.")); break;
		}
		ShowDataError(nCtrlID);
	}

	// override from CWinDataExchange
	void OnDataExchangeError(UINT nCtrlID, BOOL bSave)
	{
		m_tiText.Format(_T("Please enter a value."));
		ShowDataError(nCtrlID);
	}

	// new method; call this function to display a custom error message
	void OnDataCustomError(UINT nCtrlID, const CString& errorMsg)
	{
		m_tiText = errorMsg;
		ShowDataError(nCtrlID);
	}

private:
	void CreateToolTip()
	{
		T* pT = static_cast<T*>(this);
		m_tip.Create(*pT, NULL, NULL, TTS_NOPREFIX | TTS_NOFADE | TTS_BALLOON);
		m_tip.SetMaxTipWidth( 0 );
		m_tip.Activate( TRUE );
		CToolInfo ti(TTF_IDISHWND | TTF_SUBCLASS | TTF_TRACK, *pT, 0, 0, (LPTSTR)(LPCTSTR)m_tiText);
		m_tip.SetTitle(0, _T("Incorrect value"));
		m_tip.SetMaxTipWidth(300);
		m_tip.AddTool(&ti);
		m_ti = ti;
	}

	void ShowDataError(UINT nCtrlID) 
	{
		T* pT = static_cast<T*>(this);
		::MessageBeep((UINT)-1);
		::SetFocus(pT->GetDlgItem(nCtrlID));
		CEdit(pT->GetDlgItem(nCtrlID)).SetSelAll();
		CRect r;
		pT->GetDlgItem(nCtrlID).GetWindowRect(&r);
		if(!m_tip.IsWindow())
			CreateToolTip();
		m_ti.lpszText = (LPTSTR)(LPCTSTR)m_tiText;
		m_tip.TrackActivate(&m_ti, FALSE);
		m_tip.DelTool(&m_ti);
		m_tip.AddTool(&m_ti);
		m_tip.TrackPosition((pT->GetDlgItem(nCtrlID).GetExStyle() & WS_EX_RIGHT) ? (r.right - 8) : (r.left + 8), r.bottom - 8);
		m_tip.TrackActivate(&m_ti, TRUE);
	}

	CToolTipCtrl m_tip;
	TOOLINFO m_ti;
	CString m_tiText;
};
