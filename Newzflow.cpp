#include "stdafx.h"
#include "Newzflow.h"
#include "DiskWriter.h"
#include "PostProcessor.h"
#include "sock.h"
#include "Util.h"

// TODO:
// - CNzbView/CFileView: implement progress bar with no theming

/*static*/ CNewzflow* CNewzflow::s_pInstance = NULL;

CNewzflow::CNewzflow()
{
	ASSERT(s_pInstance == NULL);
	s_pInstance = this;

	VERIFY(CNntpSocket::InitWinsock());

	diskWriter = new CDiskWriter;
	postProcessor = new CPostProcessor;
}

CNewzflow::~CNewzflow()
{
	Util::print("waiting for disk writer to finish...\n");
	diskWriter->PostQuitMessage();
	diskWriter->Join();
	delete diskWriter;

	Util::print("waiting for post processor to finish...\n");
	postProcessor->PostQuitMessage();
	postProcessor->Join();
	delete postProcessor;

	CNntpSocket::CloseWinsock();

	s_pInstance = NULL;
}

CNewzflow* CNewzflow::Instance()
{
	return s_pInstance;
}

CNzbSegment* CNewzflow::GetSegment()
{
	CLock lock;
	for(size_t i = 0; i < nzbs.GetCount(); i++) {
		CNzb* nzb = nzbs[i];
		if(nzb->status != kQueued && nzb->status != kDownloading)
			continue;
		for(size_t j = 0; j < nzb->files.GetCount(); j++) {
			CNzbFile* file = nzb->files[j];
			if(file->status != kQueued && file->status != kDownloading)
				continue;
			for(size_t k = 0; k < file->segments.GetCount(); k++) {
				CNzbSegment* segment = file->segments[k];
				if(segment->status != kQueued)
					continue;
				segment->status = file->status = nzb->status = kDownloading;
				return segment;
			}
		}
	}
	return NULL;
}

void CNewzflow::UpdateSegment(CNzbSegment* s, ENzbStatus newStatus)
{
	ASSERT(newStatus == kError || newStatus == kCompleted);

	CLock lock;
	s->status = newStatus;
	TRACE(_T("UpdateSegment(%s, %s)\n"), s->msgId, GetNzbStatusString(newStatus));
	CNzbFile* file = s->parent;
	CNzb* nzb = file->parent;
	for(size_t i = 0; i < file->segments.GetCount(); i++) {
		CNzbSegment* segment = file->segments[i];
		if(segment->status != kCompleted && segment->status != kError) {
			return;
		}
	}
	file->status = kCompleted;

	for(size_t i = 0; i < nzb->files.GetCount(); i++) {
		CNzbFile* f = nzb->files[i];
		if(f->status != kCompleted && f->status != kError)
			return;
	}
	nzb->status = kCompleted;

	postProcessor->Add(nzb);
}
