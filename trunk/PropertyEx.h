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
	HGLOBAL m_hDlgRes;

	CPropertyPageImplEx(ATL::_U_STRINGorID title = (LPCTSTR)NULL) : 
		CPropertyPageImpl< T, TBase >(title), m_hDlgRes(NULL)
	{
		// copy from CAxPropertyPageImpl
		T* pT = static_cast<T*>(this);
		pT;   // avoid level 4 warning

		HINSTANCE hInstance = ModuleHelper::GetResourceInstance();
		LPCTSTR lpTemplateName = MAKEINTRESOURCE(pT->IDD);
		HRSRC hDlg = ::FindResource(hInstance, lpTemplateName, (LPTSTR)RT_DIALOG);
		if(hDlg != NULL)
		{
			m_hDlgRes = ::LoadResource(hInstance, hDlg);
			DLGTEMPLATE* pDlg = (DLGTEMPLATE*)::LockResource(m_hDlgRes);

			// set up property page to use in-memory dialog template
			m_psp.dwFlags |= PSP_DLGINDIRECT;
			m_psp.pResource = pDlg;

			SetMessageFontDlgTemplate(m_psp.pResource, NULL);
		}
		else
		{
			ATLASSERT(FALSE && _T("CPropertyPageEx - Cannot find dialog template!"));
		}
	}

	~CPropertyPageImplEx()
	{
		if(m_hDlgRes != NULL)
		{
			UnlockResource(m_hDlgRes);
			FreeResource(m_hDlgRes);
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


template <class T, class TBase = CPropertySheetWindow>
class CPropertySheetImplEx : public CPropertySheetImpl<T, TBase>
{
public:
	CPropertySheetImplEx(ATL::_U_STRINGorID title = (LPCTSTR)NULL, UINT uStartPage = 0, HWND hWndParent = NULL)
		: CPropertySheetImpl<T, TBase>(title, uStartPage, hWndParent) {}

	static int CALLBACK PropSheetCallback(HWND hWnd, UINT uMsg, LPARAM lParam)
	{
		if(uMsg == PSCB_PRECREATE) {
			SetMessageFontDlgTemplate((const DLGTEMPLATE*)lParam, hWnd);
		}

		return CPropertySheetImpl<T, TBase>::PropSheetCallback(hWnd, uMsg, lParam);
	}
};
