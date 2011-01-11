#include "stdafx.h"
#include "Settings.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// CSettings
//////////////////////////////////////////////////////////////////////////

CSettings::CSettings()
{
	// create %appdata%\Newzflow
	VERIFY(SHGetSpecialFolderPath(NULL, m_appData.GetBuffer(MAX_PATH), CSIDL_APPDATA, TRUE));
	m_appData.ReleaseBuffer();
	m_appData += _T("\\Newzflow\\");
	CreateDirectory(m_appData, NULL);
	m_ini = m_appData + _T("Newzflow.ini");

	// if .ini doesn't exist, create a new one with Unicode BOM, so that all subsequent INI functions operate in unicode mode
	if(!CFile::FileExists(m_ini)) {
		CFile f;
		f.Open(m_ini, GENERIC_WRITE, 0, CREATE_ALWAYS);
		unsigned char bom[] = { 0xff, 0xfe };
		f.Write(bom, 2);
	}

	// get program dir
	GetModuleFileName(_Module.GetModuleInstance(), m_programDir.GetBuffer(MAX_PATH), MAX_PATH);
	m_programDir.ReleaseBuffer();
	m_programDir = m_programDir.Left(m_programDir.ReverseFind('\\')); // remove name of executable
	if(m_programDir.Right(6) == _T("\\Debug")) m_programDir = m_programDir.Left(m_programDir.GetLength() - 6); // remove Debug
	if(m_programDir.Right(8) == _T("\\Release")) m_programDir = m_programDir.Left(m_programDir.GetLength() - 8); // remove Release

	// get download dir history
	for(int i = 0; i < 10; i++) {
		CString sKey;
		sKey.Format(_T("DownloadDir%d"), i);
		CString sDir = GetIni(_T("History"), sKey);
		if(!sDir.IsEmpty())
			downloadDirHistory.Add(sDir);
	}
}

CSettings::~CSettings()
{
	for(size_t i = 0; i < 10; i++) {
		CString sKey;
		sKey.Format(_T("DownloadDir%d"), i);
		SetIni(_T("History"), sKey, i < downloadDirHistory.GetCount() ? downloadDirHistory[i] : _T(""));
	}
}

CRect CSettings::GetWindowPos(int& showCmd)
{
	// Get Window Rect
	CString s = GetIni(_T("Window"), _T("Pos"), _T("0 0 0 0"));
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
	SetIni(_T("Window"), _T("Pos"), s);
	s.Format(_T("%d"), showCmd);
	SetIni(_T("Window"), _T("ShowCmd"), s);
}

int CSettings::GetVertSplitPos()
{
	return GetPrivateProfileInt(_T("Window"), _T("Split"), 300, m_ini);
}

void CSettings::SetVertSplitPos(int split)
{
	SetIni(_T("Window"), _T("Split"), split);
}

int CSettings::GetHorzSplitPos()
{
	return GetPrivateProfileInt(_T("Window"), _T("SplitH"), 150, m_ini);
}

void CSettings::SetHorzSplitPos(int split)
{
	SetIni(_T("Window"), _T("SplitH"), split);
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

	SetIni(name, _T("NumColumns"), numColumns);
	WritePrivateProfileStruct(name, _T("ColumnOrder"), columnOrder, numColumns * sizeof(int), m_ini);
	WritePrivateProfileStruct(name, _T("ColumnWidths"), columnWidths, numColumns * sizeof(int), m_ini);
	WritePrivateProfileStruct(name, _T("ColumnVisibility"), columnVisibility, maxColumns * sizeof(int), m_ini);
}

void CSettings::GetListViewSort(const CString& name, int& sortColumn, bool& sortAsc)
{
	int _sortColumn = _ttoi(GetIni(name, _T("SortColumn"), _T("-1")));
	bool _sortAsc = !!_ttoi(GetIni(name, _T("SortAsc"), _T("1")));

	if(_sortColumn != -1) {
		sortColumn = _sortColumn;
		sortAsc = _sortAsc;
	}
}

void CSettings::SetListViewSort(const CString& name, int sortColumn, bool sortAsc)
{
	SetIni(name, _T("SortColumn"), sortColumn);
	SetIni(name, _T("SortAsc"), sortAsc);
}

const CString& CSettings::GetAppDataDir()
{
	return m_appData;
}

const CString& CSettings::GetProgramDir()
{
	return m_programDir;
}

CString CSettings::GetIni(LPCTSTR section, LPCTSTR key, LPCTSTR def)
{
	CString val;
	DWORD ret = GetPrivateProfileString(section, key, def, val.GetBuffer(1024), 1024, m_ini);
	val.ReleaseBuffer(ret);

	return val;
}

void CSettings::SetIni(LPCTSTR section, LPCTSTR key, LPCTSTR value)
{
	DWORD ret = WritePrivateProfileString(section, key, value, m_ini);
}

void CSettings::SetIni(LPCTSTR section, LPCTSTR key, int value)
{
	CString s;
	s.Format(_T("%d"), value);
	DWORD ret = WritePrivateProfileString(section, key, s, m_ini);
}

int CSettings::GetSpeedLimit()
{
	return _ttoi(GetIni(_T("Server"), _T("MaxSpeed"), _T("0")));
}

void CSettings::SetSpeedLimit(int limit)
{
	SetIni(_T("Server"), _T("MaxSpeed"), limit);
}

int CSettings::GetConnections()
{
	return min(100, max(1, _ttoi(GetIni(_T("Server"), _T("Connections"), _T("10")))));
}

void CSettings::SetConnections(int connections)
{
	SetIni(_T("Server"), _T("Connections"), connections);
}

CString CSettings::GetDownloadDir()
{
	if(GetIni(_T("Directories"), _T("UseDownload"), _T("0")) != _T("0"))
		return GetIni(_T("Directories"), _T("Download"), _T(""));
	else
		return _T("");
}

CString CSettings::GetWatchDir()
{
	if(GetIni(_T("Directories"), _T("UseWatch"), _T("0")) != _T("0"))
		return GetIni(_T("Directories"), _T("Watch"), _T(""));
	else
		return _T("");
}

bool CSettings::GetDeleteWatch()
{
	return GetIni(_T("Directories"), _T("DeleteWatch"), _T("0")) != _T("0");
}

CString CSettings::GetMovieQualityName(int q)
{
	static const LPCTSTR names[] = {
		_T("Blu-Ray ISO"),
		_T("x264 1080p"),
		_T("x264 720p"),
		_T("WMV 1080p"),
		_T("WMV 720p"),
		_T("DVD ISO"),
		_T("Xvid"),
	};

	if(q >= 0 && q < mqMax)
		return names[q];
	else
		return CString();
}
