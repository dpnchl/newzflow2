// aboutdlg.cpp : implementation of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "Version.h"

#include "AboutDlg.h"

static int extraHeight = 0;

static BOOL CALLBACK WndEnumProc(HWND hWnd, LPARAM lParam)
{
	CRect r;
	GetWindowRect(hWnd, &r);
	CPoint p(r.TopLeft());
	ScreenToClient(GetParent(hWnd), &p);
	SetWindowPos(hWnd, NULL, p.x, p.y + extraHeight, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	return TRUE;
}

LRESULT CAboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CRect r;
	GetDlgItem(IDC_ABOUT).GetWindowRect(&r);
	int titleHeight = r.Height() * 2;
	extraHeight = 256 + r.Height() * 13 / 10; // make room for 256x256 icon and title text

	// resize dialog
	GetWindowRect(&r);
	SetWindowPos(NULL, 0, 0, r.Width(), r.Height() + extraHeight, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);

	// reposition all dialog controls
	EnumChildWindows(*this, WndEnumProc, (LPARAM)this);

	// Windows XP won't display our 256x256 PNG icon using CStatic(SS_ICON), so get the PNG icon from the resource manually
	// and use GDI+'s CImage to get it into a CStatic(SS_BITMAP)

	// Find and load appropriate icon (IDR_MAINFRAME, 256x256 pixels)
	HINSTANCE hInstance = ModuleHelper::GetResourceInstance();
	HRSRC hDlg = ::FindResource(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME), (LPTSTR)RT_GROUP_ICON);
	HGLOBAL hRes = ::LoadResource(hInstance, hDlg);
	int nID = LookupIconIdFromDirectoryEx((PBYTE)hRes, TRUE, 256, 256, LR_DEFAULTCOLOR); 
	hDlg = ::FindResource(hInstance, MAKEINTRESOURCE(nID), MAKEINTRESOURCE(RT_ICON));
	DWORD sRes = ::SizeofResource(hInstance, hDlg);
	hRes = ::LoadResource(hInstance, hDlg);
	char* pRes = (char*)::LockResource(hRes);

	// can't create a IStream directly on the resource, so make a copy of the PNG
	HGLOBAL	hMem = ::GlobalAlloc(GMEM_MOVEABLE, sRes);
	ASSERT(hMem);
	LPVOID pImage = ::GlobalLock(hMem);
	memcpy(pImage, pRes, sRes);
	::GlobalUnlock(hMem);

	// Load the PNG via a IStream operating on the copy of the PNG icon
	CComPtr<IStream> stream;
	HRESULT hr = CreateStreamOnHGlobal(hMem, FALSE, &stream);
	ASSERT(SUCCEEDED(hr));
	hr = m_image.Load(stream);
	ASSERT(SUCCEEDED(hr));
	::GlobalFree(hMem);

	// Finally create a CStatic with SS_BITMAP and set the bitmap
	CStatic icon;
	icon.Create(*this, CRect(CPoint(0, 0), CSize(r.Width(), 256)), NULL, WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_REALSIZEIMAGE | SS_CENTERIMAGE);
	icon.SetBitmap(m_image);

	CStatic title;
	title.Create(*this, CRect(CPoint(0, 256), CSize(r.Width(), titleHeight)), NULL, WS_CHILD | WS_VISIBLE | SS_CENTER);
	title.SetWindowText(_T("Newzflow"));

	LOGFONT lf;
	CFontHandle font = GetDlgItem(IDC_ABOUT).GetFont();
	font.GetLogFont(lf);
	lf.lfHeight *= 2;
	lf.lfWeight = FW_BOLD;
	m_font.CreateFontIndirect(&lf);
	title.SetFont(m_font);

	// Set License Text
	GetDlgItem(IDC_LICENSE).SetWindowText(_T("This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.\r\n\r\nThis program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.\r\n\r\nYou should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>."));
	CString sAbout;
	sAbout.Format(_T("build %s (%s)"), _T(NEWZFLOW_REVISION), _T(NEWZFLOW_DATE));
	GetDlgItem(IDC_ABOUT).SetWindowText(sAbout);

	CenterWindow(GetParent());
	return TRUE;
}

LRESULT CAboutDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}

LRESULT CAboutDlg::OnClickURL(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	PNMLINK pNMLink = (PNMLINK)pnmh;
	LITEM item = pNMLink->item;
	if(item.iLink == 0) {
		ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
	}
	return 0;
}