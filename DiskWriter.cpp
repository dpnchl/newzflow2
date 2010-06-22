#include "stdafx.h"
#include "util.h"
#include "DiskWriter.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

void CDiskWriter::Add(const CString& file, __int64 offset, void* buffer, unsigned int size)
{
	CJob* job = new CJob;
	job->file = file;
	job->offset = offset;
	job->buffer = buffer;
	job->size = size;

	PostThreadMessage(MSG_JOB, (WPARAM)job);
}

LRESULT CDiskWriter::OnJob(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CJob* job = (CJob*)wParam;

	CString s;
	s.Format(_T("ObJob(%s, %I64d, %d)"), job->file, job->offset, job->size);
	Util::Print(CStringA(s));

	CFile file;
	if(file.Open(job->file, GENERIC_WRITE, 0, OPEN_ALWAYS)) {
		file.Seek(job->offset, FILE_BEGIN);
		file.Write(job->buffer, job->size);
		file.Close();
	}
	delete job->buffer;
	delete job;

	return 0;
}