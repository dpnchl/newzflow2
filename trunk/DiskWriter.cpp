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

	CString nzbDir;
	nzbDir.Format(_T("%s%s"), CNewzflow::Instance()->settings->GetAppDataDir(), CComBSTR(file->parent->guid));

	CFile fout;
	fout.Open(file->parent->path + file->fileName, GENERIC_WRITE, 0, CREATE_ALWAYS);

	// join all segments from temp dir to final file
	for(size_t i = 0; i < file->segments.GetCount(); i++) {
		CNzbSegment* s = file->segments[i];
		CString segFileName;
		segFileName.Format(_T("%s\\%s_part%05d"), nzbDir, file->fileName, s->number);
		CFile fin;
		if(fin.Open(segFileName, GENERIC_READ, 0, OPEN_ALWAYS)) {
			int size = (int)fin.GetSize();
			char* buffer = new char [size];
			fin.Read(buffer, size);
			fin.Close();
			CFile::Delete(segFileName); // TODO: do only if writing everything succeeded
			fout.Write(buffer, size);
			delete buffer;
		}
	}

	fout.Close();
	CNewzflow::Instance()->UpdateFile(file, kCompleted);

	return 0;
}