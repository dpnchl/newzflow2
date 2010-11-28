#pragma once

// use CPropertyPageImplEx instead of CPropertyPageImpl; CPropertyPageEx instead of CPropertyPage
// and CPropertySheetImplEx instead of CPropertySheetImpl
// to automatically replace the property sheet's and property pages' font with NONCLIENTMETRICS.MessageFont (Segoe UI on Vista/Win7)

#include "DialogTemplate.h"

namespace {
	// Idea from http://www.codeguru.com/cpp/controls/propertysheet/previoussectionmanager/article.php/c16651/Custom-Font-in-Property-Sheets.htm
	static inline void SetMessageFontDlgTemplate(const DLGTEMPLATE* pResource, HWND hWnd)
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

			SetMessageFontDlgTemplate(m_psp.pResource, NULL);
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
			SetMessageFontDlgTemplate((const DLGTEMPLATE*)lParam, hWnd);
		}

		return CPropertySheetImpl<T, TBase>::PropSheetCallback(hWnd, uMsg, lParam);
	}
};
