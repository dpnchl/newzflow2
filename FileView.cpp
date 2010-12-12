// FileView.cpp : implementation of the CFileView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "FileView.h"
#include "Newzflow.h"
#include "Util.h"
#include "Settings.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// columns
enum {
	kName,
	kSize,
	kDone,
	kProgress,
	kSegments,
	kStatus,
	kParStatus,
};

/*static*/ const CFileView::ColumnInfo CFileView::s_columnInfo[] = { 
	{ _T("Name"),		_T("Name"),			CFileView::typeString,	LVCFMT_LEFT,	400,	true },
	{ _T("Size"),		_T("Size"),			CFileView::typeSize,	LVCFMT_RIGHT,	80,		true },
	{ _T("Done"),		_T("Done"),			CFileView::typeNumber,	LVCFMT_RIGHT,	80,		true },
	{ _T("Progress"),	_T("Progress"),		CFileView::typeNumber,	LVCFMT_RIGHT,	150,	true },
	{ _T("# Segments"),	_T("# Seg"),		CFileView::typeNumber,	LVCFMT_RIGHT,	50,		true },
	{ _T("Status"),		_T("Status"),		CFileView::typeString,	LVCFMT_LEFT,	120,	true },
	{ _T("PAR Status"),	_T("PAR Status"),	CFileView::typeString,	LVCFMT_LEFT,	120,	true },
	{ NULL }
};

CFileView::CFileView()
{
	nzb = NULL;
}

BOOL CFileView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

void CFileView::Init(HWND hwndParent)
{
	Create(hwndParent, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS, 0);

	m_thmProgress.OpenThemeData(*this, L"PROGRESS");

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(*this, L"explorer", NULL);

	InitDynamicColumns(_T("FileView"));
}

void CFileView::SetNzb(CNzb* _nzb)
{
	if(_nzb != nzb) {
		nzb = _nzb;
		Refresh();
	}
}

int CFileView::OnRefresh()
{
	if(!nzb || nzb->status == kEmpty || nzb->status == kFetching)
		return 0;

	class CLine {
	public:
		CString name;
		DWORD_PTR file;
		__int64 size, done;
		int segments;
		ENzbStatus status;
		EParStatus parStatus;
		float parDone;
	};

	CAtlArray<CLine> lines;

	{ NEWZFLOW_LOCK;
		CNewzflow* theApp = CNewzflow::Instance();
		size_t count = nzb->files.GetCount();
		for(size_t i = 0; i < count; i++) {
			CNzbFile* file = nzb->files[i];
			CLine line;
			line.name = file->fileName;
			line.file = (DWORD_PTR)file;
			line.size = 0; 
			line.done = 0;
			line.status = file->status;
			for(size_t j = 0; j < file->segments.GetCount(); j++) {
				line.size += file->segments[j]->bytes;
				if(file->segments[j]->status == kCompleted || file->segments[j]->status == kCached)
					line.done += file->segments[j]->bytes;
				if(file->segments[j]->status == kDownloading) // check if any segment is downloading
					line.status = kDownloading;
			}
			line.segments = file->segments.GetCount();
			line.parStatus = file->parStatus;
			line.parDone = file->parDone;
			lines.Add(line);

/*			CString s = file->subject;
			AddItemEx(i, (DWORD_PTR)file);
			SetItemTextEx(i, kName, s);
			__int64 size = 0, done = 0;
			ENzbStatus status = file->status;
			for(size_t j = 0; j < file->segments.GetCount(); j++) {
				size += file->segments[j]->bytes;
				if(file->segments[j]->status == kCompleted || file->segments[j]->status == kCached)
					done += file->segments[j]->bytes;
				if(file->segments[j]->status == kDownloading) // check if any segment is downloading
					status = kDownloading;
			}
			SetItemTextEx(i, kSize, Util::FormatSize(size));
			SetItemTextEx(i, kDone, Util::FormatSize(done));
			s.Format(_T("%.1f %%"), 100.f * (float)done / (float)size);
			SetItemTextEx(i, kProgress, s);
			s.Format(_T("%d"), file->segments.GetCount());
			SetItemTextEx(i, kSegments, s);
			SetItemTextEx(i, kStatus, GetNzbStatusString(status));
			SetItemTextEx(i, kParStatus, GetParStatusString(file->parStatus, file->parDone));
*/
		}

		for(size_t i = 0; i < lines.GetCount(); i++) {
			CLine& line = lines[i];
			AddItemEx(i, line.file);
			SetItemTextEx(i, kName, line.name);
			SetItemTextEx(i, kSize, Util::FormatSize(line.size));
			SetItemTextEx(i, kDone, Util::FormatSize(line.done));
			CString s;
			s.Format(_T("%.1f %%"), 100.f * (float)line.done / (float)line.size);
			SetItemTextEx(i, kProgress, s);
			s.Format(_T("%d"), line.segments);
			SetItemTextEx(i, kSegments, s);
			SetItemTextEx(i, kStatus, GetNzbStatusString(line.status));
			SetItemTextEx(i, kParStatus, GetParStatusString(line.parStatus, line.parDone));
		}

		return count;
	}
}

const CFileView::ColumnInfo* CFileView::GetColumnInfoArray()
{
	return s_columnInfo;
}

LRESULT CFileView::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	Refresh();

	return 0;
}

LRESULT CFileView::OnRClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
	if(m_menu.IsNull())
		m_menu.LoadMenu(IDR_FILEVIEW);

	DWORD lParam = GetMessagePos();
	CPoint ptScreen(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	bool canPause = false, canUnpause = false;

	{ NEWZFLOW_LOCK;
		for(int item = GetNextItem(-1, LVNI_SELECTED); item != -1; item = GetNextItem(item, LVNI_SELECTED)) {
			CNzbFile* file = (CNzbFile*)GetItemData(item);
			if(file->status == kQueued)
				canPause = true;
			if(file->status == kPaused)
				canUnpause = true;
		}
	}

	m_menu.GetSubMenu(0).EnableMenuItem(ID_FILE_PAUSE, (canPause ? MF_ENABLED : MF_DISABLED) | MF_BYCOMMAND);
	m_menu.GetSubMenu(0).EnableMenuItem(ID_FILE_UNPAUSE, (canUnpause ? MF_ENABLED : MF_DISABLED) | MF_BYCOMMAND);
	m_menu.GetSubMenu(0).TrackPopupMenu(TPM_RIGHTBUTTON, ptScreen.x, ptScreen.y, *this);

	return 0;
}

LRESULT CFileView::OnFilePause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	{ NEWZFLOW_LOCK;
		for(int item = GetNextItem(-1, LVNI_SELECTED); item != -1; item = GetNextItem(item, LVNI_SELECTED)) {
			CNzbFile* file = (CNzbFile*)GetItemData(item);
			if(file->status == kQueued)
				file->status = kPaused;
		}
	}
	SendMessage(WM_TIMER);
	// we should send the main window WM_TIMER to refresh, but don't know how to get a handle to the main window
	return 0;
}

LRESULT CFileView::OnFileUnpause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	{ NEWZFLOW_LOCK;
		for(int item = GetNextItem(-1, LVNI_SELECTED); item != -1; item = GetNextItem(item, LVNI_SELECTED)) {
			CNzbFile* file = (CNzbFile*)GetItemData(item);
			if(file->status == kPaused)
				file->status = kQueued;
		}
	}
	CNewzflow::Instance()->CreateDownloaders();
	SendMessage(WM_TIMER);
	// we should send the main window WM_TIMER to refresh, but don't know how to get a handle to the main window
	return 0;
}

// CCustomDraw
DWORD CFileView::OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
{
	return CDRF_NOTIFYITEMDRAW;
}

DWORD CFileView::OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
{
	return CDRF_NOTIFYSUBITEMDRAW;
}

class CTempDC : public CDC
{
public:
// Data members
	HDC m_hDCOriginal;
	RECT m_rcPaint;
	CBitmap m_bmp;
	HBITMAP m_hBmpOld;

// Constructor/destructor
	CTempDC(HDC hDC, RECT& rcPaint) : m_hDCOriginal(hDC), m_hBmpOld(NULL)
	{
		m_rcPaint = rcPaint;
		CreateCompatibleDC(m_hDCOriginal);
		ATLASSERT(m_hDC != NULL);
		m_bmp.CreateCompatibleBitmap(m_hDCOriginal, m_rcPaint.right - m_rcPaint.left, m_rcPaint.bottom - m_rcPaint.top);
		ATLASSERT(m_bmp.m_hBitmap != NULL);
		m_hBmpOld = SelectBitmap(m_bmp);
		SetViewportOrg(-m_rcPaint.left, -m_rcPaint.top);
	}

	~CTempDC()
	{
		SelectBitmap(m_hBmpOld);
	}
};


DWORD CFileView::OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW lpNMCustomDraw)
{
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)lpNMCustomDraw;
	if(cd->iSubItem == SubItemFromColumn(kProgress)) {
		CRect rc;
		GetSubItemRect(cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);
		if(rc.Width() <= 10)
			return CDRF_DODEFAULT;

		CMemoryDC dcMem(cd->nmcd.hdc, rc);
		CTempDC dcOk(cd->nmcd.hdc, rc), dcErr(cd->nmcd.hdc, rc);

		dcMem.BitBlt(rc.left, rc.top, rc.Width(), rc.Height(), cd->nmcd.hdc, rc.left, rc.top, SRCCOPY);

		CRect rcProgress(rc);
		rcProgress.DeflateRect(3, 2);
		m_thmProgress.DrawThemeBackground(dcMem, PP_BAR, 0, rcProgress, NULL);
		m_thmProgress.DrawThemeBackground(dcOk, PP_BAR, 0, rcProgress, NULL);
		m_thmProgress.DrawThemeBackground(dcErr, PP_BAR, 0, rcProgress, NULL);

		m_thmProgress.DrawThemeBackground(dcOk, PP_FILL, PBFS_NORMAL, rcProgress, NULL);
		m_thmProgress.DrawThemeBackground(dcErr, PP_FILL, PBFS_ERROR, rcProgress, NULL);

		if(rcProgress.Width() == 0)
			return CDRF_DODEFAULT;


/*

bytesPerSlice = 10;
offset = 3;
segBytes = 12;

sliceStartIndex = 3 / 10 = 0
sliceEndIndex = 15 / 10 = 1
sliceEndOffset = 10
sliceEndOffset - offset = 10 - 3 = 7
bytesInStartSlice = 

0    10    20    30
|     |     |     |
  ########
  3      15

*/

		int width = rcProgress.Width();
		float* slicesOk = NULL, *slicesErr = NULL;
		float bytesPerSlice = 0.f;
		{ NEWZFLOW_LOCK;
			CNzbFile* file = (CNzbFile*)GetItemData(cd->nmcd.dwItemSpec);
			__int64 totalSize = 0;
			for(size_t i = 0; i < file->segments.GetCount(); i++) {
				totalSize += file->segments[i]->bytes;
			}
			bytesPerSlice = (float)totalSize / (float)width;
			if(bytesPerSlice < 1.f)
				return CDRF_DODEFAULT;
			slicesOk = new float[width];
			slicesErr = new float[width];
			for(int i = 0; i < width; i++) {
				slicesOk[i] = slicesErr[i] = 0.f;
			}
			__int64 offset = 0;
			for(size_t i = 0; i < file->segments.GetCount(); i++) {
				CNzbSegment* seg = file->segments[i];
				if(seg->status == kCompleted || seg->status == kCached) {
					float internalOffset = (float)offset;
					float internalBytes = (float)seg->bytes;
					int sliceStartIndex = (int)((float)internalOffset / bytesPerSlice);
					int sliceEndIndex = (int)((float)(internalOffset + internalBytes) / bytesPerSlice);
					float sliceEndOffset = ((sliceStartIndex + 1) * bytesPerSlice);
					while(sliceEndIndex >= sliceStartIndex && internalBytes > 0 && sliceStartIndex < width) {
						float bytesInStartSlice = min(sliceEndOffset - internalOffset, internalBytes);
						slicesOk[sliceStartIndex] += bytesInStartSlice;
						internalOffset += bytesInStartSlice;
						internalBytes -= bytesInStartSlice;
						sliceStartIndex++;
						sliceEndOffset += bytesPerSlice;
					}
				} else if(seg->status == kError) {
					float internalOffset = (float)offset;
					float internalBytes = (float)seg->bytes;
					int sliceStartIndex = (int)((float)internalOffset / bytesPerSlice);
					int sliceEndIndex = (int)((float)(internalOffset + internalBytes) / bytesPerSlice);
					float sliceEndOffset = ((sliceStartIndex + 1) * bytesPerSlice);
					while(sliceEndIndex >= sliceStartIndex && internalBytes > 0 && sliceStartIndex < width) {
						float bytesInStartSlice = min(sliceEndOffset - internalOffset, internalBytes);
						slicesErr[sliceStartIndex] += bytesInStartSlice;
						internalOffset += bytesInStartSlice;
						internalBytes -= bytesInStartSlice;
						sliceStartIndex++;
						sliceEndOffset += bytesPerSlice;
					}
				}
				offset += seg->bytes;
			}
		}

		for(int i = 0; i < width; i++) {
			slicesOk[i] /= bytesPerSlice;
			slicesErr[i] /= bytesPerSlice;
			BLENDFUNCTION bf;
			bf.BlendOp = AC_SRC_OVER;
			bf.BlendFlags = 0;
			bf.SourceConstantAlpha = min(255, (int)(slicesOk[i] * 255.f));
			bf.AlphaFormat = 0;
			dcMem.AlphaBlend(rcProgress.left + i, rcProgress.top, 1, rcProgress.Height(), dcOk, rcProgress.left + i, rcProgress.top, 1, rcProgress.Height(), bf);
			bf.SourceConstantAlpha = min(255, (int)(slicesErr[i] * 255.f));
			dcMem.AlphaBlend(rcProgress.left + i, rcProgress.top, 1, rcProgress.Height(), dcErr, rcProgress.left + i, rcProgress.top, 1, rcProgress.Height(), bf);
		}

		delete[] slicesOk;
		delete[] slicesErr;

		return CDRF_SKIPDEFAULT;
	}
	return CDRF_DODEFAULT;
}
