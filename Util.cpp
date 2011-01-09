#include "stdafx.h"
#include "util.h"
#include <fcntl.h>
#include <io.h>
#include "HttpDownloader.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

namespace Util
{
	void CreateConsole()
	{
		static bool consoleOpen = false;
		if(consoleOpen)
			return;

		// redirect unbuffered STDOUT to the console
		int hConHandle;
		long lStdHandle;
		FILE *fp;
		AllocConsole();
		lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
		hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
		fp = _fdopen( hConHandle, "w" );
		*stdout = *fp;
		setvbuf( stdout, NULL, _IONBF, 0 );
		// redirect unbuffered STDIN to the console
		lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
		hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
		fp = _fdopen( hConHandle, "r" );
		*stdin = *fp;
		setvbuf( stdin, NULL, _IONBF, 0 );
		// redirect unbuffered STDERR to the console
		lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
		hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
		fp = _fdopen( hConHandle, "w" );
		*stderr = *fp;
		setvbuf( stderr, NULL, _IONBF, 0 );
		consoleOpen = true;
	}

	void Print(const char* s)
	{
		Print(CString(s));
	}

	void Print(const wchar_t* s)
	{
		CString str;
		str.Format(L"[%4x] %s%s", GetCurrentThreadId(), s, wcschr(s, '\n') ? L"" : L"\n");
		OutputDebugString(str);
	}

	CString FormatSize(__int64 size)
	{
		static TCHAR prefix[] = _T("kMGT");
		CString s;
		if(size < 1024)
			s.Format(_T("%I64d B"), size);
		else {
			int idx = -1;
			double dsize = (double)size;
			while(dsize >= 1024.0 && idx+1 < 4 /*sizeof(prefix)/sizeof(prefix[0])*/) {
				idx++;
				dsize /= 1024.0;
			}
			if(dsize < 10.0)
				s.Format(_T("%.2f %cB"), floor(dsize * 100.0) / 100.0, prefix[idx]);
			else if(dsize < 100.0)
				s.Format(_T("%.1f %cB"), floor(dsize * 10.0) / 10.0, prefix[idx]);
			else
				s.Format(_T("%.0f %cB"), dsize, prefix[idx]);
		}
		return s;
	}

	__int64 ParseSize(const CString& s)
	{
		double f1 = _tstof(s);
		if(s.Find('k') >= 0) f1 *= 1024.;
		else if(s.Find('M') >= 0) f1 *= 1024. * 1024.;
		else if(s.Find('G') >= 0) f1 *= 1024. * 1024. * 1024.;
		else if(s.Find('T') >= 0) f1 *= 1024. * 1024. * 1024. * 1024.;
		return (__int64)f1;
	}

	CString FormatSpeed(__int64 speed)
	{
		return FormatSize(speed) + _T("/s");
	}

	CString FormatTimeSpan(__int64 span) // span is in seconds
	{
		static const __int64 minute = 60;
		static const __int64 hour = minute * 60;
		static const __int64 day = hour * 24;
		static const __int64 week = day * 7;
		CString s;
		int weeks = (int)(span / week);
		int days = (span / day) % 7;
		int hours = (span / hour) % 24;
		int minutes = (span / minute) % 60;
		int seconds = span % 60;
		if(weeks > 0)
			s.Format(_T("%dw %dd"), weeks, days);
		else if(days > 0)
			s.Format(_T("%dd %dh"), days, hours);
		else if(hours > 0)
			s.Format(_T("%dh %dm"), hours, minutes);
		else
			s.Format(_T("%dm %ds"), minutes, seconds);
		return s;
	}

	__int64 ParseTimeSpan(const CString& s)
	{
		static const __int64 minute = 60;
		static const __int64 hour = minute * 60;
		static const __int64 day = hour * 24;
		static const __int64 week = day * 7;

		if(s == _T("\x221e"))
			return LLONG_MAX;

		TCHAR* end = NULL;
		__int64 v1 = _tcstoul(s, &end, 10);
		if(end && *end) {
			if(*end == 'm') v1 *= minute;
			else if(*end == 'h') v1 *= hour;
			else if(*end == 'd') v1 *= day;
			else if(*end == 'w') v1 *= week;

			end++;
			__int64 v2 = _tcstoul(end, &end, 10);
			if(*end == 'm') v2 *= minute;
			else if(*end == 'h') v2 *= hour;
			else if(*end == 'd') v2 *= day;
			else if(*end == 'w') v2 *= week;

			return v1 + v2;
		}

		return v1;
	}

	CString SanitizeFilename(const CString& fn)
	{
		CString out;
		static const TCHAR* reserved = _T("<>:/\\|?*");
		for(int i = 0; i < fn.GetLength(); i++) {
			TCHAR c = fn[i];
			if(c > 31 && !_tcschr(reserved, c))
				out += c;
		}
		return out;
	}

	void DeleteDirectory(const CString& _path)
	{
		CString path(_path);
		if(path.Right(1) != _T("\\")) path += '\\';
		WIN32_FIND_DATA fdata;
		HANDLE h = FindFirstFile(path + _T("*"), &fdata);
		if(h != INVALID_HANDLE_VALUE) {
			do {
				if(!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					CFile::Delete(path + fdata.cFileName);
			} while(FindNextFile(h, &fdata));
		}
		FindClose(h);
		RemoveDirectory(_path);
	}

	void RegisterAssoc(HINSTANCE hInstance)
	{
		CRegKey reg;
		reg.Create(HKEY_CURRENT_USER, _T("Software\\Classes\\Newzflow.1"));
		reg.SetStringValue(NULL, _T("Newzflow NZB"));
		reg.Close();

		reg.Create(HKEY_CURRENT_USER, _T("Software\\Classes\\Newzflow.1\\Content Type"));
		reg.SetStringValue(NULL, _T("application/x-nzb"));
		reg.Close();

		CString sAppPath;
		::GetModuleFileName(hInstance, sAppPath.GetBuffer(MAX_PATH), MAX_PATH); sAppPath.ReleaseBuffer();
		CString sIcon;
		sIcon.Format(_T("\"%s\",0"), sAppPath);
		reg.Create(HKEY_CURRENT_USER, _T("Software\\Classes\\Newzflow.1\\DefaultIcon"));
		reg.SetStringValue(NULL, sIcon);
		reg.Close();

		CString sOpenCmd;
		sOpenCmd.Format(_T("\"%s\" \"%%1\""), sAppPath);
		reg.Create(HKEY_CURRENT_USER, _T("Software\\Classes\\Newzflow.1\\shell\\open\\command"));
		reg.SetStringValue(NULL, sOpenCmd);
		reg.Close();

		reg.Create(HKEY_CURRENT_USER, _T("Software\\Classes\\.nzb"));
		reg.SetStringValue(NULL, _T("Newzflow.1"));
		reg.SetStringValue(_T("Content Type"), _T("application/x-nzb"));
		reg.Close();

		reg.Create(HKEY_CURRENT_USER, _T("Software\\Classes\\.nzb\\OpenWithProgids"));
		reg.SetStringValue(_T("Newzflow.1"), _T(""));
		reg.Close();

		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	}

	void UnregisterAssoc()
	{
		CRegKey reg;
		reg.Open(HKEY_CURRENT_USER, _T("Software\\Classes"));
		reg.RecurseDeleteKey(_T("Newzflow.1"));
		reg.RecurseDeleteKey(_T(".nzb"));

		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	}

	CString BrowseForFolder(HWND hwndParent, const TCHAR* title, const TCHAR* initialDir)
	{
		// first try new Vista dialog
		CShellFileOpenDialog dlg(NULL, FOS_PICKFOLDERS | FOS_PATHMUSTEXIST);
		if(dlg.GetPtr()) {
			CComPtr<IShellItem> psiFolder;
			// dynamically load SHCreateItemFromParsingName from shell32.dll; if we would just use the statically imported version, we couldn't run on Windows XP anymore
			typedef HRESULT (STDAPICALLTYPE *SHCREATEITEMFROMPARSINGNAME)(__in PCWSTR, __in_opt IBindCtx *, __in REFIID, __deref_out void **);
			SHCREATEITEMFROMPARSINGNAME imp_SHCreateItemFromParsingName = (SHCREATEITEMFROMPARSINGNAME)::GetProcAddress(::LoadLibrary(_T("shell32.dll")), "SHCreateItemFromParsingName");
			// set initial folder
			if(initialDir) {
				HRESULT hr = imp_SHCreateItemFromParsingName(CStringW(initialDir), NULL, IID_PPV_ARGS(&psiFolder));
				if(SUCCEEDED(hr))
					dlg.GetPtr()->SetFolder(psiFolder);
			}
			if(title)
				dlg.GetPtr()->SetTitle(title);
			if(dlg.DoModal(hwndParent) == IDOK) {
				CStringW sReturnW;
				dlg.GetFilePath(sReturnW.GetBuffer(_MAX_PATH), _MAX_PATH);
				sReturnW.ReleaseBuffer();
				// partial workaround for Vista http://support.microsoft.com/kb/969885/en-us
				if(GetFileAttributesW(sReturnW) == INVALID_FILE_ATTRIBUTES) {
					int slash = sReturnW.ReverseFind('\\');
					if(slash >= 0)
						sReturnW = sReturnW.Left(slash);
				}
				return CString(sReturnW);
			} else
				return initialDir;
		} else {
			// fallback to XP dialog
			CFolderDialog dlg(hwndParent, title, BIF_RETURNONLYFSDIRS | BIF_USENEWUI);
			if(initialDir)
				dlg.SetInitialFolder(initialDir);
			if(dlg.DoModal() == IDOK)
				return dlg.GetFolderPath();
			else
				return initialDir;
		}
	}

	static CWindow mainWindow;
	CWindow GetMainWindow()
	{
		return mainWindow;
	}

	void SetMainWindow(CWindow wnd)
	{
		mainWindow = wnd;
	}

	void PostMessageToMainWindow(UINT msg, WPARAM wParam /*= 0*/, LPARAM lParam /*= 0*/)
	{
		if(GetMainWindow().IsWindow())
			GetMainWindow().PostMessage(msg, wParam, lParam);
	}

	CString GetErrorMessage(int errCode)
	{
		LPTSTR errString = NULL;  // will be allocated and filled by FormatMessage
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, errCode, 0, (LPTSTR)&errString, 0, 0);

		CString s(errString);
		LocalFree(errString);

		return s;
	}

	int TestCreateDirectory(const CString& path)
	{
		// check if path is valid; try to create
		int result = SHCreateDirectoryEx(NULL, path, NULL);
		if(result != ERROR_SUCCESS && result != ERROR_ALREADY_EXISTS && result != ERROR_FILE_EXISTS) {
			return result;
		}

		// if we created a new path, remove it
		if(result != ERROR_ALREADY_EXISTS && result != ERROR_FILE_EXISTS)
			RemoveDirectory(path);

		return result;
	}

	bool Shutdown(EShutdownMode mode)
	{
		if(mode == shutdown_nothing)
			return true;
		if(mode == shutdown_exit) {
			GetMainWindow().PostMessage(WM_CLOSE);
			return true;
		}

		HANDLE hToken; 
		TOKEN_PRIVILEGES tkp; 

		// Get a token for this process. 
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
			return false; 

		// Get the LUID for the shutdown privilege. 
		LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid); 

		tkp.PrivilegeCount = 1;  // one privilege to set    
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

		// Get the shutdown privilege for this process. 

		AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0); 
		if (GetLastError() != ERROR_SUCCESS) 
			return false; 

		// Shut down the system and force all applications to close. 

		switch(mode) {
		case shutdown_poweroff:
			return ExitWindowsEx(EWX_SHUTDOWN | EWX_POWEROFF, SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE | SHTDN_REASON_FLAG_PLANNED) != 0;
		case shutdown_suspend:
			return SetSuspendState(FALSE, FALSE, FALSE) != 0;
		case shutdown_hibernate:
			return SetSuspendState(TRUE, FALSE, FALSE) != 0;
		default:
			ASSERT(FALSE);
			return false;
		}
	}

	namespace {
		struct SGroupCheck {
			CRect groupRect;
			BOOL enable;
			HWND check;
		};

		BOOL CALLBACK CheckDlgGroupCallback(HWND hWnd, LPARAM lParam)
		{
			SGroupCheck* gc = (SGroupCheck*)lParam;
			CRect r;
			GetWindowRect(hWnd, &r);
			if(hWnd != gc->check && gc->groupRect.PtInRect(r.TopLeft()) && gc->groupRect.PtInRect(r.BottomRight()))
				EnableWindow(hWnd, gc->enable);
			return TRUE;
		}
	} // namespace

	void CheckDlgGroup(HWND hDlg, HWND hCheck, HWND hGroup)
	{
		SGroupCheck gc;
		GetWindowRect(hGroup, &gc.groupRect);
		gc.enable = SendMessage(hCheck, BM_GETCHECK, 0, 0);
		gc.check = hCheck;

		// reposition all dialog controls
		EnumChildWindows(hDlg, CheckDlgGroupCallback, (LPARAM)&gc);
	}
};

// CToolBarImageList
//////////////////////////////////////////////////////////////////////////

CToolBarImageList::~CToolBarImageList()
{
	ilNormal.Destroy();
	ilDisabled.Destroy();
}

bool CToolBarImageList::Load(const CString& path, int cx, int cy)
{
	CImage image;
	if(!SUCCEEDED(image.Load(path)))
		return false;
	return Load(image, cx, cy);
}

bool CToolBarImageList::LoadFromResource(ATL::_U_STRINGorID bitmap, int cx, int cy)
{
	CImage image;
	image.LoadFromResource(ModuleHelper::GetResourceInstance(), bitmap.m_lpstr);
	return Load(image, cx, cy);
}

bool CToolBarImageList::Load(CImage &image, int cx, int cy)
{
	if(image.GetBPP() != 32)
		return false;

	ilNormal.Create(cx, cy, ILC_COLOR32, 0, 100);
	ilNormal.Add((HBITMAP)image, (HBITMAP)NULL);

	// generate disabled image by applying 50% alpha and 75% desaturation
	unsigned char* bits = (unsigned char*)image.GetBits();
	int pitch = image.GetPitch();
	for(int y = 0; y < image.GetHeight(); y++) {
		for(int x = 0; x < image.GetWidth(); x++) {
			unsigned char* pp = &bits[x*4];
			int grey = ((unsigned)pp[0] + (unsigned)pp[1] + (unsigned)pp[2]) / 3;
			int r = pp[0] + ((grey - pp[0]) * 3 >> 2);
			int g = pp[1] + ((grey - pp[1]) * 3 >> 2);
			int b = pp[2] + ((grey - pp[2]) * 3 >> 2);
			pp[0] = r;
			pp[1] = g;
			pp[2] = b;
			pp[3] >>= 1;
		}
		bits += pitch;
	}
	ilDisabled.Create(cx, cy, ILC_COLOR32, 0, 100);
	ilDisabled.Add((HBITMAP)image, (HBITMAP)NULL);

	return true;
}

void CToolBarImageList::Set(HWND hwndToolBar)
{
	CToolBarCtrl toolBar(hwndToolBar);

	toolBar.SetImageList(ilNormal);
	toolBar.SetDisabledImageList(ilDisabled);
}

// CImageListEx
//////////////////////////////////////////////////////////////////////////

BOOL CImageListEx::Create(int _cx, int _cy, bool _crop)
{
	cx = _cx; cy = _cy;
	crop = _crop;
	return CImageList::Create(cx, cy, ILC_COLOR32, 0, 200);
}

int CImageListEx::AddEmpty()
{
	CImage imgEmpty;
	imgEmpty.Create(cx, cy, 32);
	unsigned char* bits = (unsigned char*)imgEmpty.GetBits();
	int pitch = imgEmpty.GetPitch();
	for(int y = 0; y < imgEmpty.GetHeight(); y++) {
		memset(bits, 255, imgEmpty.GetWidth() * imgEmpty.GetBPP() / 8);
		bits += pitch;
	}
	return CImageList::Add((HBITMAP)imgEmpty, (HBITMAP)NULL);
}

int CImageListEx::Add(const CString& path)
{
	CImage image;
	if(FAILED(image.Load(path)))
		return -1;

	int ocx = image.GetWidth(), ocy = image.GetHeight();
	float oaspect = (float)ocx / (float)ocy;
	float aspect = (float)cx / (float)cy;

	// resize image
	CImage img1; 
	img1.Create(cx, cy, 32); 
	CDC dc; 
	dc.Attach(img1.GetDC()); 
	dc.SetStretchBltMode(HALFTONE); 
	if(crop) {
		if(oaspect < aspect) {
			// crop top&bottom
			image.StretchBlt(dc, 0, (int)(cy - (float)cx / oaspect) / 2, cx, (int)(cx / oaspect), SRCCOPY); 
		} else {
			// crop left&right
			image.StretchBlt(dc, (int)(cx - (float)cy * oaspect) / 2, 0, (int)(cy * oaspect), cy, SRCCOPY); 
		}
	} else {
		image.StretchBlt(dc, 0, 0, cx, cy, SRCCOPY); 
	}
	dc.Detach(); 
	img1.ReleaseDC(); 
	return CImageList::Add((HBITMAP)img1, (HBITMAP)NULL);
}

int CImageListEx::AddIcon(HICON icon, COLORREF gradientTop /*= ~0*/, COLORREF gradientBottom /*= ~0*/)
{
	CImage imgEmpty;
	imgEmpty.Create(cx, cy, 32);
	CDC dc;
	dc.Attach(imgEmpty.GetDC());

	// Create an array of TRIVERTEX structures that describe 
	// positional and color values for each vertex. For a rectangle, 
	// only two vertices need to be defined: upper-left and lower-right. 
	TRIVERTEX vertex[2] ;
	vertex[0].x     = 0;
	vertex[0].y     = 0;
	vertex[0].Red   = GetRValue(gradientTop) << 8;
	vertex[0].Green = GetGValue(gradientTop) << 8;
	vertex[0].Blue  = GetBValue(gradientTop) << 8;
	vertex[0].Alpha = (gradientTop >> 24) << 8;

	vertex[1].x     = cx;
	vertex[1].y     = cy; 
	vertex[1].Red   = GetRValue(gradientBottom) << 8;
	vertex[1].Green = GetGValue(gradientBottom) << 8;
	vertex[1].Blue  = GetBValue(gradientBottom) << 8;
	vertex[1].Alpha = (gradientBottom >> 24) << 8;

	// Create a GRADIENT_RECT structure that 
	// references the TRIVERTEX vertices. 
	GRADIENT_RECT gRect;
	gRect.UpperLeft  = 0;
	gRect.LowerRight = 1;

	// Draw a shaded rectangle. 
	dc.GradientFill(vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);

	dc.DrawIcon((cx-32)/2, (cy-32)/2, icon); // doesn't work on XP; why?
	dc.Detach();
	imgEmpty.ReleaseDC();
	return CImageList::Add((HBITMAP)imgEmpty, (HBITMAP)NULL);
}

// CAsyncDownloaderThread
//////////////////////////////////////////////////////////////////////////

class CAsyncDownloaderThread : public CThreadImpl<CAsyncDownloaderThread>
{
public:

	CAsyncDownloaderThread(CAsyncDownloader* pDownloader)
	: CThreadImpl<CAsyncDownloaderThread>(CREATE_SUSPENDED)
	{
		m_pDownloader = pDownloader;
		Resume();
	}
	~CAsyncDownloaderThread()
	{
	}

protected:
	DWORD Run()
	{
		downloader.Init();
		while(1) {
			HANDLE objs[] = { m_pDownloader->shutDown, m_pDownloader->refresh };
			if(WAIT_OBJECT_0 == WaitForMultipleObjects(2, objs, FALSE, INFINITE))
				break;

			// refresh
			while(1) {
				if(WAIT_OBJECT_0 == WaitForSingleObject(m_pDownloader->shutDown, 0))
					break;
				CAsyncDownloader::CItem* item = NULL;
				m_pDownloader->cs.Lock();
				if(!m_pDownloader->queue.IsEmpty()) {
					item = m_pDownloader->queue.RemoveHead();
					m_pDownloader->finished.AddTail(item);
				} else {
					m_pDownloader->refresh.Reset();
				}
				m_pDownloader->cs.Unlock();

				if(item) {
					if(downloader.Download(item->url, item->path, CString(), NULL)) {
						if(IsWindow(m_pDownloader->sink)) {
							PostMessage(m_pDownloader->sink, m_pDownloader->msg, (WPARAM)item, (LPARAM)0);
						}
					}
				} else {
					break;
				}
			}
		}

		return 0;
	}

protected:
	CHttpDownloader downloader;

	CAsyncDownloader* m_pDownloader;
};


// CAsyncDownloader
//////////////////////////////////////////////////////////////////////////

CAsyncDownloader::CAsyncDownloader() : shutDown(TRUE, FALSE)
, refresh(TRUE, FALSE)
{
	sink = NULL;
	msg = 0;
}

CAsyncDownloader::~CAsyncDownloader()
{
	shutDown.Set();
	CWaitCursor w;
	for(size_t i = 0; i < threads.GetCount(); i++) {
		threads[i]->Join();
		delete threads[i];
	}
	while(!queue.IsEmpty()) delete queue.RemoveHead();
	while(!finished.IsEmpty()) delete finished.RemoveHead();
}

void CAsyncDownloader::Init(int numThreads)
{
	cs.Lock();
	while((int)threads.GetCount() < numThreads) {
		CAsyncDownloaderThread* thread = new CAsyncDownloaderThread(this);
		threads.Add(thread);
	}
	cs.Unlock();
}

void CAsyncDownloader::SetSink(HWND _sink, int _msg)
{
	sink = _sink;
	msg = _msg;
}

void CAsyncDownloader::Add(const CString& url, const CString& path, DWORD_PTR param /*= 0*/)
{
	CAsyncDownloader::CItem* item = new CAsyncDownloader::CItem;
	item->url = url;
	item->path = path;
	item->param = param;

	cs.Lock();
	queue.AddTail(item);
	refresh.Set();
	cs.Unlock();
}

void CAsyncDownloader::Clear()
{
	cs.Lock();
	while(!queue.IsEmpty()) delete queue.RemoveHead();
	cs.Unlock();
}
