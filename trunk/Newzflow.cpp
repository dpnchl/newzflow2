#include "stdafx.h"
#include "Newzflow.h"

/*static*/ CNewzflow* CNewzflow::s_pInstance = NULL;

CNewzflow::CNewzflow()
{
	ASSERT(s_pInstance == NULL);
	s_pInstance = this;
}

CNewzflow::~CNewzflow()
{
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
	CLock lock;
	s->status = newStatus;
	TRACE(_T("UpdateSegment(%s, %s)\n"), s->msgId, GetNzbStatusString(newStatus));
	CNzbFile* file = s->parent;
	CNzb* nzb = file->parent;
	for(size_t i = 0; i < file->segments.GetCount(); i++) {
		CNzbSegment* segment = file->segments[i];
		if(segment->status != newStatus) {
			return;
		}
	}

	file->status = newStatus;
	for(size_t i = 0; i < nzb->files.GetCount(); i++) {
		CNzbFile* f = nzb->files[i];
		if(f->status != newStatus)
			return;
	}
	nzb->status = newStatus;
}
