#include "stdafx.h"
#include "Nzb.h"
#include "DownloadQueue.h"

/*static*/ CDownloadQueue* CDownloadQueue::s_pInstance = NULL;

void CDownloadQueue::Add(CNzbFile* nzbfile)
{
	CComCritSecLock<CComAutoCriticalSection> lock(cs);
	for(size_t i = 0; i < nzbfile->segments.GetCount(); i++) {
		CEntry* e = new CEntry;
		e->file = nzbfile;
		e->segment = nzbfile->segments[i];
		list.AddTail(e);
	}
}

CDownloadQueue::CEntry* CDownloadQueue::Get()
{
	CComCritSecLock<CComAutoCriticalSection> lock(cs);
	if(list.IsEmpty())
		return NULL;
	return list.RemoveHead();
}