// FileView.cpp : implementation of the CFileView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "FileView.h"
#include "Newzflow.h"
#include "Util.h"

CFileView::CFileView()
{
	nzb = NULL;
}

BOOL CFileView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

// columns
enum {
	kName,
	kSize,
	kDone,
	kStatus,
	kSegments,
};

void CFileView::Init(HWND hwndParent)
{
	Create(hwndParent, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS, 0);

	m_thmProgress.OpenThemeData(*this, L"PROGRESS");

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(*this, L"explorer", NULL);
	AddColumn(_T("Name"), kName);
	AddColumn(_T("Size"), kSize, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	AddColumn(_T("Done"), kDone, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	AddColumn(_T("%"), kStatus, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	AddColumn(_T("# Seg"), kSegments, -1, LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_RIGHT);
	SetColumnWidth(kName, 400);
	SetColumnWidth(kSize, 80);
	SetColumnWidth(kDone, 80);
	SetColumnWidth(kStatus, 150);
}

void CFileView::SetNzb(CNzb* _nzb)
{
	if(_nzb != nzb) {
		nzb = _nzb;
		BOOL b;
		OnTimer(0, 0, 0, b);
	}
}

LRESULT CFileView::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if(!nzb) {
		if(GetItemCount() > 0)
			DeleteAllItems();
		return 0;
	}

	int lvCount = GetItemCount();

	{ CNewzflow::CLock lock;
		CNewzflow* theApp = CNewzflow::Instance();
		size_t count = nzb->files.GetCount();
		for(size_t i = 0; i < count; i++) {
			CNzbFile* file = nzb->files[i];
			CString s = file->subject;
			if(i >= lvCount) {
				AddItem(i, kName, s);
			} else {
				SetItemText(i, kName, s);
			}
			__int64 size = 0, done = 0;
			for(size_t j = 0; j < file->segments.GetCount(); j++) {
				size += file->segments[j]->bytes;
				if(file->segments[j]->status == kCompleted)
					done += file->segments[j]->bytes;
			}
			AddItem(i, kSize, Util::FormatSize(size));
			AddItem(i, kDone, Util::FormatSize(done));
			s.Format(_T("%.1f %%"), 100.f * (float)done / (float)size);
			AddItem(i, kStatus, s);
			s.Format(_T("%d"), file->segments.GetCount());
			AddItem(i, kSegments, s);
			SetItemData(i, (DWORD_PTR)file);
		}
		SetRedraw(FALSE);
		while(count < lvCount) {
			DeleteItem(count);
			lvCount--;
		}
		SetRedraw(TRUE);
	}

	return 0;
}

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
	if(cd->iSubItem == kStatus) {
		CRect rc(cd->nmcd.rc);
		if(rc.Width() <= 10)
			return CDRF_DODEFAULT;

		CMemoryDC dcMem(cd->nmcd.hdc, cd->nmcd.rc);
		CTempDC dcOk(cd->nmcd.hdc, cd->nmcd.rc), dcErr(cd->nmcd.hdc, cd->nmcd.rc);

		dcMem.BitBlt(rc.left, rc.top, rc.Width(), rc.Height(), cd->nmcd.hdc, rc.left, rc.top, SRCCOPY);

		CRect rcProgress(cd->nmcd.rc);
		rcProgress.DeflateRect(3, 2);
		m_thmProgress.DrawThemeBackground(dcMem, PP_BAR, 0, rcProgress, NULL);
		m_thmProgress.DrawThemeBackground(dcOk, PP_BAR, 0, rcProgress, NULL);
		m_thmProgress.DrawThemeBackground(dcErr, PP_BAR, 0, rcProgress, NULL);

		m_thmProgress.DrawThemeBackground(dcOk, PP_FILL, PBFS_NORMAL, rcProgress, NULL);
		m_thmProgress.DrawThemeBackground(dcErr, PP_FILL, PBFS_ERROR, rcProgress, NULL);

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

		{ CNewzflow::CLock lock;
			CNzbFile* file = (CNzbFile*)GetItemData(cd->nmcd.dwItemSpec);
			int width = rcProgress.Width();
			float* slicesOk = new float [width], *slicesErr = new float[width];
			for(int i = 0; i < width; i++) {
				slicesOk[i] = slicesErr[i] = 0.f;
			}
			__int64 totalSize = 0;
			for(int i = 0; i < file->segments.GetCount(); i++) {
				totalSize += file->segments[i]->bytes;
			}
			float bytesPerSlice = (float)totalSize / (float)width;
			__int64 offset = 0;
			for(int i = 0; i < file->segments.GetCount(); i++) {
				CNzbSegment* seg = file->segments[i];
				if(seg->status == kCompleted) {
					float internalOffset = (float)offset;
					float internalBytes = (float)seg->bytes;
					int sliceStartIndex = (int)((float)internalOffset / bytesPerSlice);
					int sliceEndIndex = (int)((float)(internalOffset + internalBytes) / bytesPerSlice);
					float sliceEndOffset = ((sliceStartIndex + 1) * bytesPerSlice);
					while(sliceEndIndex >= sliceStartIndex && internalBytes > 0) {
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
					while(sliceEndIndex >= sliceStartIndex && internalBytes > 0) {
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
		}

		return CDRF_SKIPDEFAULT;
	}
	return CDRF_DODEFAULT;
}
