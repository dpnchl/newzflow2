// LogView.cpp : implementation of the CLogView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "LogView.h"
#include "Newzflow.h"
#include "Util.h"
#include "Settings.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

CLogView::CLogView()
{
	nzb = NULL;
}

BOOL CLogView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

void CLogView::Init(HWND hwndParent)
{
	Create(hwndParent, rcDefault, NULL, WS_CHILD | WS_VSCROLL | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL, 0);
	SetReadOnly();
}

void CLogView::SetNzb(CNzb* _nzb)
{
	if(_nzb != nzb) {
		nzb = _nzb;
		Refresh();
	}
}

LRESULT CLogView::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	Refresh();

	return 0;
}

void CLogView::Refresh()
{
	CString log;
	if(nzb) { 
		NEWZFLOW_LOCK;
		log = nzb->log;
	}
	if(log != oldLog) {
		CString s;
		CString rtfLog;
		int logLen = log.GetLength();
		// split log by lines; convert "newline" and color failures
		for(int i = 0; i < logLen; ) {
			int next = log.Find(_T("\r\n"), i);
			if(next == -1)
				next = log.GetLength();

			CString line = log.Mid(i, next - i);
			if(line.Find(_T("failed")) >= 0)
				rtfLog += _T("\\cf2 ");
			else
				rtfLog += _T("\\cf1 ");
			rtfLog += line;
			rtfLog += _T("\\par ");

			i = next + 2;
		}
		s = _T("{\\rtf1\\ansi\\deff0{\\colortbl;\\red0\\green0\\blue0;\\red128\\green0\\blue0;}") + rtfLog + _T("}");
		// convert RTF from Unicode to UTF8.
		int utfLen = s.GetLength() * 4;
		char* utf = new char[utfLen];
		::WideCharToMultiByte(CP_UTF8, 0, (LPCTSTR)s, -1, utf, utfLen-1, NULL, NULL);
		SetTextEx((LPCTSTR)utf, ST_DEFAULT, CP_UTF8);
		delete utf;
		oldLog = log;
	}
}