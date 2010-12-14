#include "stdafx.h"
#include "util.h"
#include "DirWatcher.h"
#include "Newzflow.h"
#include "Settings.h"
#include "MemFile.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// CDirWatcher
//////////////////////////////////////////////////////////////////////////

CDirWatcher::CDirWatcher()
: CThreadImpl<CDirWatcher>(CREATE_SUSPENDED)
, shutDown(TRUE, FALSE)
{
	Resume();
}

DWORD CDirWatcher::Run()
{
	int delay = 0;

	ReadIgnoreList();

	for(;;) {
		if(CNewzflow::Instance()->IsShuttingDown() || WaitForSingleObject(shutDown, 0) == WAIT_OBJECT_0)
			break;

		if(delay > 0) {
			WaitForSingleObject(shutDown, delay * 1000); // either waits until the delay ran out, or we need to shut down
			delay = 0;
			continue;
		}

		ProcessDirectory(CNewzflow::Instance()->settings->GetWatchDir());

		delay = 5;
	}

	WriteIgnoreList();

	return 0;
}

void CDirWatcher::ProcessDirectory(const CString& dir)
{
	if(dir.IsEmpty())
		return;

	// scan directory for .nzb files
	WIN32_FIND_DATA wfd;
	HANDLE h = FindFirstFile(dir + _T("\\*"), &wfd);
	if(h == INVALID_HANDLE_VALUE)
		return;

	CAtlArray<CString> filesToCheck;
	do {
		// skip everything that's not a file
		if(wfd.dwFileAttributes & (FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN))
			continue;

		// filter extension
		CString s(wfd.cFileName);
		s.MakeLower();
		if(s.Right(4) != _T(".nzb"))
			continue;

		// skip if in filesToIgnore
		bool ignore = false;
		for(size_t i = 0; i < filesToIgnore.GetCount(); i++) {
			if(!filesToIgnore[i].CompareNoCase(wfd.cFileName)) {
				ignore = true;
				break;
			}
		}
		if(ignore)
			continue;

		// skip if we can't get exclusive access (probably some other application is still writing)
		CFile ftest;
		if(!ftest.Open(dir + _T("\\") + wfd.cFileName, GENERIC_READ, 0, OPEN_EXISTING))
			continue;

		filesToCheck.Add(wfd.cFileName);
	} while(FindNextFile(h, &wfd));
	FindClose(h);

	for(size_t i = 0; i < filesToCheck.GetCount(); i++) {
		CNzb* nzb = new CNzb;
		if(nzb->CreateFromPath(dir + _T("\\") + filesToCheck[i])) {
			CNewzflow::Instance()->controlThread->AddNZB(nzb);
			if(CNewzflow::Instance()->settings->GetDeleteWatch())
				CFile::Delete(dir + _T("\\") + filesToCheck[i]);
			else
				filesToIgnore.Add(filesToCheck[i]);
		} else {
			delete nzb;
		}
	}
}

// TODO: also store file modification date!

void CDirWatcher::ReadIgnoreList()
{
	CFile f;
	if(f.Open(CNewzflow::Instance()->settings->GetAppDataDir() + _T("watcher.dat"), GENERIC_READ, 0, OPEN_EXISTING)) {
		size_t size = (size_t)f.GetSize();
		void* buffer = new char [size];
		f.Read(buffer, size);
		f.Close();

		CMemFile mf(buffer, size);
		if(mf.Read<unsigned>() != 'NFW!') {
			return;
		}
		unsigned version = mf.Read<unsigned>();
		if(version != 1) {
			return;
		}

		unsigned count = mf.Read<unsigned>();
		while(count != 0) {
			filesToIgnore.Add(mf.ReadString());
			count--;
		}
	}
}

void CDirWatcher::WriteIgnoreList()
{
	CMemFile mf(128*1024, 128*1024);
	size_t count = filesToIgnore.GetCount();
	mf.Write<unsigned>('NFW!'); // ID
	mf.Write<unsigned>(1); // Version
	mf.Write<size_t>(count);
	for(size_t i = 0; i < count; i++) {
		mf.WriteString(filesToIgnore[i]);
	}
	CFile f;
	DeleteFile(CNewzflow::Instance()->settings->GetAppDataDir() + _T("watcher.dat"));
	if(count) {
		if(f.Open(CNewzflow::Instance()->settings->GetAppDataDir() + _T("watcher.dat"), GENERIC_WRITE, 0, CREATE_ALWAYS)) {
			f.Write(mf.GetBuffer(), mf.GetSize());
			f.Close();
		}
	}
}