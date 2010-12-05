#include "stdafx.h"
#include "util.h"
#include "DiskWriter.h"
#include "Newzflow.h"
#include "Settings.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

void CDiskWriter::Add(CNzbFile* file)
{
	{ CNewzflow::CLock lock;
		file->parent->refCount++;
	}

	PostThreadMessage(MSG_JOB, (WPARAM)file);
}

LRESULT CDiskWriter::OnJob(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CNzbFile* file = (CNzbFile*)wParam;

	// we can only append consecutive segments to the destination file
	int maxSegment = -1;
	bool fileExists = false;
	for(size_t i = 0; i < file->segments.GetCount(); i++) {
		if(file->segments[i]->status == kCompleted)
			fileExists = true;
		if(file->segments[i]->status == kQueued || file->segments[i]->status == kDownloading)
			break;
		if(file->segments[i]->status == kCached)
			maxSegment = (int)i;
	}

	// got new segments to append?
	if(maxSegment >= 0) {
		CString nzbDir;
		nzbDir.Format(_T("%s%s"), CNewzflow::Instance()->settings->GetAppDataDir(), CComBSTR(file->parent->guid));

		CFile fout;
		if(!fileExists) {
			SHCreateDirectoryEx(NULL, file->parent->path, NULL);
			fout.Open(file->parent->path + _T("\\") + file->fileName, GENERIC_WRITE, 0, CREATE_ALWAYS);
		} else {
			fout.Open(file->parent->path + _T("\\") + file->fileName, FILE_APPEND_DATA, 0, OPEN_ALWAYS);
		}
		if(fout.IsOpen()) { // TODO: Error check
			// join all segments from temp dir to final file
			for(int i = 0; i <= maxSegment; i++) {
				CNzbSegment* s = file->segments[i];
				if(s->status == kCached) { // kError segments will be skipped
					CString segFileName;
					segFileName.Format(_T("%s\\%s_part%05d"), nzbDir, file->fileName, s->number);
					CFile fin;
					if(fin.Open(segFileName, GENERIC_READ, 0, OPEN_ALWAYS)) {
						int size = (int)fin.GetSize();
						char* buffer = new char [size];
						fin.Read(buffer, size);
						fin.Close();
						CFile::Delete(segFileName); // TODO: do only if writing everything succeeded
						s->status = kCompleted;
						fout.Write(buffer, size);
						delete buffer;
					}
				}
			}
			fout.Close();
			CString s;
			s.Format(_T("DiskWriter: processed until segment %d for %s"), maxSegment+1, file->fileName);
			Util::Print(s);
		}

		// file finished?
		if(maxSegment == file->segments.GetCount()-1) {
			CNewzflow::Instance()->UpdateFile(file, kCompleted);
			return 0; // UpdateFile() decrements refCount
		}
	}
	CNewzflow::CLock lock;
	file->parent->refCount--;
	return 0;
}
