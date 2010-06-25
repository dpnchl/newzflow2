#include "stdafx.h"
#include "Newzflow.h"
#include "DiskWriter.h"
#include "Downloader.h"
#include "PostProcessor.h"
#include "sock.h"
#include "Util.h"
#include "Settings.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// TODO:
// - CNzbView/CFileView: implement progress bar with no theming

// CNewzflowThread
//////////////////////////////////////////////////////////////////////////

void CNewzflowThread::AddFile(const CString& nzbPath)
{
	if(nzbPath.IsEmpty())
		return;

	CString nzbUrl;
	// make a full path (incl. drive and directory) for XML Parser's URL
	if(!(nzbPath.GetLength() > 1 && nzbPath[1] == ':')) {
		CString curDir;
		::GetCurrentDirectory(512, curDir.GetBuffer(512)); curDir.ReleaseBuffer();
		if(nzbPath[0] == '\\') {
			// full directory specified, but drive omitted
			nzbUrl = curDir.Left(2) + nzbPath;
		} else {
			// relative file/directory specified
			nzbUrl = curDir + _T("\\") + nzbPath;
		}
	} else {
		// full path specified
		nzbUrl = nzbPath;
	}
	nzbUrl.Replace('\\', '/');
	nzbUrl = _T("file://") + nzbUrl;

	PostThreadMessage(MSG_JOB, (WPARAM)new CString(nzbUrl));
}

void CNewzflowThread::AddURL(const CString& nzbUrl)
{
	PostThreadMessage(MSG_JOB, (WPARAM)new CString(nzbUrl));
}

LRESULT CNewzflowThread::OnJob(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CString* nzbUrl = (CString*)wParam;

	CNzb* nzb = CNzb::Create(*nzbUrl);
	if(nzb) {
		{ CNewzflow::CLock lock;
			CNewzflow::Instance()->nzbs.Add(nzb);

			for(size_t i = 0; i < CNewzflow::Instance()->finishedDownloaders.GetCount(); i++) {
				delete CNewzflow::Instance()->finishedDownloaders[i];
			}
			CNewzflow::Instance()->finishedDownloaders.RemoveAll();

			int numDownloaders = max(0, 5 - CNewzflow::Instance()->downloaders.GetCount());
			for(int i = 0; i < numDownloaders; i++) {
				CNewzflow::Instance()->downloaders.Add(new CDownloader);
			}
		}
	}

	delete nzbUrl;
	return 0;
}

/*static*/ CNewzflow* CNewzflow::s_pInstance = NULL;

// CNewzflow
//////////////////////////////////////////////////////////////////////////

CNewzflow::CNewzflow()
{
	ASSERT(s_pInstance == NULL);
	s_pInstance = this;

	VERIFY(CNntpSocket::InitWinsock());

	shuttingDown = false;
	diskWriter = new CDiskWriter;
	postProcessor = new CPostProcessor;
	controlThread = new CNewzflowThread;
	settings = new CSettings;
}

CNewzflow::~CNewzflow()
{
	shuttingDown = true;

	Util::Print("waiting for control thread to finish...\n");
	controlThread->PostQuitMessage();
	controlThread->Join();
	delete controlThread;

	Util::Print("waiting for downloaders to finish...\n");
	for(size_t i = 0; i < downloaders.GetCount(); i++) {
		downloaders[i]->Join();
		delete downloaders[i];
	}
	downloaders.RemoveAll();

	for(size_t i = 0; i < finishedDownloaders.GetCount(); i++) {
		delete finishedDownloaders[i];
	}
	finishedDownloaders.RemoveAll();

	Util::Print("waiting for disk writer to finish...\n");
	diskWriter->PostQuitMessage();
	diskWriter->Join();
	delete diskWriter;

	Util::Print("waiting for post processor to finish...\n");
	postProcessor->PostQuitMessage();
	postProcessor->Join();
	delete postProcessor;

	for(size_t i = 0; i < nzbs.GetCount(); i++) {
		delete nzbs[i];
	}
	nzbs.RemoveAll();

	delete settings;

	CNntpSocket::CloseWinsock();

	s_pInstance = NULL;
	Util::Print("~CNewzflow finished\n");
}

CNewzflow* CNewzflow::Instance()
{
	return s_pInstance;
}

CNzbSegment* CNewzflow::GetSegment()
{
	CLock lock;
	if(shuttingDown)
		return NULL;

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
				nzb->refCount++;
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
	nzb->refCount--;
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
	nzb->refCount++;
	postProcessor->Add(nzb);
}

// a finished downloader calls this to remove itself from the downloaders array
// it's put into the finishedDownloaders array that will be cleaned upon AddJob and ~CNewzflow
// unfortunately a thread cannot delete itself.
void CNewzflow::RemoveDownloader(CDownloader* dl)
{
	CLock lock;
	// if we're shutting down, do nothing. ~CNewzflow will wait for all downloaders and delete them
	if(shuttingDown)
		return;

	for(size_t i = 0; i < downloaders.GetCount(); i++) {
		if(downloaders[i] == dl) {
			downloaders.RemoveAt(i);
			finishedDownloaders.Add(dl);
			break;
		}
	}
}