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
		SetWindowText(log);
		oldLog = log;
	}
}