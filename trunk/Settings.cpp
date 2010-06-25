#include "stdafx.h"
#include "Settings.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// CSettings
//////////////////////////////////////////////////////////////////////////

CSettings::CSettings()
{
	VERIFY(SHGetSpecialFolderPath(NULL, m_appData.GetBuffer(MAX_PATH), CSIDL_APPDATA, TRUE));
	m_appData.ReleaseBuffer();
	m_appData += _T("\\Newzflow\\");
	CreateDirectory(m_appData, NULL);
	m_ini = m_appData + _T("Newzflow.ini");
}

CSettings::~CSettings()
{

}

CRect CSettings::GetWindowPos(int& showCmd)
{
	// Get Window Rect
	CString s;
	GetPrivateProfileString(_T("Window"), _T("Pos"), _T("0 0 0 0"), s.GetBuffer(512), 512, m_ini);
	s.ReleaseBuffer();
	CRect r;
	if(_stscanf(s, _T("%d %d %d %d"), &r.left, &r.top, &r.right, &r.bottom) != 4)
		r.SetRectEmpty();

	// make sure the window is on the screen, otherwise reset to default
	CRect screen(0, 0, GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));
	if(!CRect().IntersectRect(r, screen))
		r.SetRectEmpty();

	// Get ShowCmd (normal/minimized/maximized)
	showCmd = GetPrivateProfileInt(_T("Window"), _T("ShowCmd"), SW_SHOWDEFAULT, m_ini);
	return r;
}

void CSettings::SetWindowPos(const CRect& r, int showCmd)
{
	CString s;
	s.Format(_T("%d %d %d %d"), r.left, r.top, r.right, r.bottom);
	WritePrivateProfileString(_T("Window"), _T("Pos"), s, m_ini);
	s.Format(_T("%d"), showCmd);
	WritePrivateProfileString(_T("Window"), _T("ShowCmd"), s, m_ini);
}

int CSettings::GetSplitPos()
{
	return GetPrivateProfileInt(_T("Window"), _T("Split"), 300, m_ini);
}

void CSettings::SetSplitPos(int split)
{
	CString s;
	s.Format(_T("%d"), split);
	WritePrivateProfileString(_T("Window"), _T("Split"), s, m_ini);
}

void CSettings::GetListViewColumns(const CString& name, CListViewCtrl lv, int* columnVisibility, int maxColumns)
{
	// ignore stored settings if column count doesn't match
	int numColumns = GetPrivateProfileInt(name, _T("NumColumns"), 0, m_ini);
	if(numColumns == 0 || numColumns > maxColumns)
		return;

	if(!GetPrivateProfileStruct(name, _T("ColumnVisibility"), columnVisibility, maxColumns * sizeof(int), m_ini))
		return;

	CTempBuffer<int, _WTL_STACK_ALLOC_THRESHOLD> buffOrder;
	int* columnOrder = buffOrder.Allocate(numColumns);
	CTempBuffer<int, _WTL_STACK_ALLOC_THRESHOLD> buffWidths;
	int* columnWidths = buffWidths.Allocate(numColumns);
	if(!GetPrivateProfileStruct(name, _T("ColumnOrder"), columnOrder, numColumns * sizeof(int), m_ini))
		return;
	if(!GetPrivateProfileStruct(name, _T("ColumnWidths"), columnWidths, numColumns * sizeof(int), m_ini))
		return;

	for(int i = maxColumns-1; i >= 0; i--) {
		if(columnVisibility[i] < 0)
			lv.DeleteColumn(i);
	}

	lv.SetColumnOrderArray(numColumns, columnOrder);
	for(int i = 0; i < numColumns; i++)
		lv.SetColumnWidth(i, columnWidths[i]);
}

void CSettings::SetListViewColumns(const CString& name, CListViewCtrl lv, int* columnVisibility, int maxColumns)
{
	// numColumns can be lower than maxColumns if some columns are hidden
	int numColumns = lv.GetHeader().GetItemCount();
	CTempBuffer<int, _WTL_STACK_ALLOC_THRESHOLD> buffOrder;
	int* columnOrder = buffOrder.Allocate(numColumns);
	lv.GetColumnOrderArray(numColumns, columnOrder);
	CTempBuffer<int, _WTL_STACK_ALLOC_THRESHOLD> buffWidths;
	int* columnWidths = buffWidths.Allocate(numColumns);
	for(int i = 0; i < numColumns; i++)
		columnWidths[i] = lv.GetColumnWidth(i);

	CString s;
	s.Format(_T("%d"), numColumns);
	WritePrivateProfileString(name, _T("NumColumns"), s, m_ini);
	WritePrivateProfileStruct(name, _T("ColumnOrder"), columnOrder, numColumns * sizeof(int), m_ini);
	WritePrivateProfileStruct(name, _T("ColumnWidths"), columnWidths, numColumns * sizeof(int), m_ini);
	WritePrivateProfileStruct(name, _T("ColumnVisibility"), columnVisibility, maxColumns * sizeof(int), m_ini);
}

const CString& CSettings::GetAppDataDir()
{
	return m_appData;
}