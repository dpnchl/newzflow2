#include "stdafx.h"
#include "util.h"
#include "DiskWriter.h"
#include "Newzflow.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

void CDiskWriter::Add(CNzbSegment* seg, void* buffer, unsigned int size)
{
	{ CNewzflow::CLock lock;
		seg->parent->parent->refCount++;
	}

	CJob* job = new CJob;
	job->segment = seg;
	job->buffer = buffer;
	job->size = size;

	PostThreadMessage(MSG_JOB, (WPARAM)job);
}

LRESULT CDiskWriter::OnJob(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CJob* job = (CJob*)wParam;

	CString s;
	s.Format(_T("CDiskWriter::OnJob(%s, %I64d, %d)"), job->segment->parent->fileName, job->segment->offset, job->size);
	Util::Print(CStringA(s));

	CFile file;
	if(file.Open(job->segment->parent->parent->path + job->segment->parent->fileName, GENERIC_WRITE, 0, OPEN_ALWAYS)) {
		file.Seek(job->segment->offset, FILE_BEGIN);
		file.Write(job->buffer, job->size);
		file.Close();
	}
	delete job->buffer;

	CNewzflow::Instance()->UpdateSegment(job->segment, kCompleted);

	delete job;

	return 0;
}