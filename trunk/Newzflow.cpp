#include "stdafx.h"
#include "resource.h"
#include "Newzflow.h"
#include "DiskWriter.h"
#include "Downloader.h"
#include "PostProcessor.h"
#include "NntpSocket.h"
#include "Util.h"
#include "Settings.h"
#include "HttpDownloader.h"
#include "DirWatcher.h"
#include "RssWatcher.h"
#include "MainFrm.h"
#include "MemFile.h"
#include "DialogEx.h"
#include "TheTvDB.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// TODO:
// - associate NZBs with RssItems to get status information back into RssView
// - ... also to get NZB filename when no sensible one is returned (e.g. binsearch.info only returns the NZB ID)
// - remove .nzb extension from NZB names/download directories
// - investigate why downloads get corrupted during testing (forcibly shutting down Newzbin lots of times)
// - investigate why sometimes no downloaders are created when par2 failed and asks for more blocks
// - CNzbView/CFileView: implement progress bar with no theming
// - CNewzflowThread:: AddFile(): what to do when a file is added that is already in the queue. can it be detected? should it be skipped or renamed (add counter) and added anyway?
// - during shut down, it should be avoided to add jobs to the disk writer or post processor; have to check if queue states are correctly restored if we just skip adding jobs.
// - investigate why .nzb files are not deleted in %appdata% sometimes
// - DirWatcher: add support for .nzb.gz and .zip, .rar
// - Centralize calls to WriteQueue()
// - There seem to be some problems resizing the window after the splitter has been dragged to extremes...
// - ViewTree is flickering when refreshing; TODO: selectively update the tree with just the changed feeds
// - CAddTvShow dialog has some drawing problems under WinXP (blue selection over images)

// CNewzflowThread
//////////////////////////////////////////////////////////////////////////

void CNewzflowThread::AddFile(const CString& nzbPath)
{
	if(nzbPath.IsEmpty())
		return;

	CString nzbUrl;
	if(nzbPath.GetLength() >= 3 && ((nzbPath[1] == ':' && nzbPath[2] == '\\') || (nzbPath[0] == '\\' && nzbPath[1] == '\\'))) {
		// full path specified
		nzbUrl = nzbPath;
	} else {
		// make a full path (incl. drive and directory) for XML Parser's URL
		CString curDir;
		::GetCurrentDirectory(512, curDir.GetBuffer(512)); curDir.ReleaseBuffer();
		if(nzbPath[0] == '\\') {
			// full directory specified, but drive omitted
			nzbUrl = curDir.Left(2) + nzbPath;
		} else {
			// relative file/directory specified
			nzbUrl = curDir + _T("\\") + nzbPath;
		}
	}

	if(GetCurrentThreadId() != GetId())
		PostThreadMessage(MSG_ADD_FILE, (WPARAM)new CString(nzbUrl));
	else {
		BOOL b;
		OnAddFile(MSG_ADD_FILE, (WPARAM)new CString(nzbUrl), 0, b);
	}
}

void CNewzflowThread::AddURL(const CString& nzbUrl)
{
	if(GetCurrentThreadId() != GetId())
		PostThreadMessage(MSG_ADD_FILE, (WPARAM)new CString(nzbUrl));
	else {
		BOOL b;
		OnAddFile(MSG_ADD_FILE, (WPARAM)new CString(nzbUrl), 0, b);
	}
}

void CNewzflowThread::AddNZB(CNzb* nzb)
{
	if(GetCurrentThreadId() != GetId())
		PostThreadMessage(MSG_ADD_NZB, (WPARAM)nzb);
	else {
		BOOL b;
		OnAddNZB(MSG_ADD_NZB, (WPARAM)nzb, 0, b);
	}
}

void CNewzflowThread::WriteQueue()
{
	if(GetCurrentThreadId() != GetId())
		PostThreadMessage(MSG_WRITE_QUEUE);
	else {
		BOOL b;
		OnWriteQueue(MSG_WRITE_QUEUE, 0, 0, b);
	}
}

void CNewzflowThread::CreateDownloaders()
{
	if(GetCurrentThreadId() != GetId())
		PostThreadMessage(MSG_CREATE_DOWNLOADERS);
	else {
		BOOL b;
		OnCreateDownloaders(MSG_CREATE_DOWNLOADERS, 0, 0, b);
	}
}

LRESULT CNewzflowThread::OnAddFile(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CString* nzbUrl = (CString*)wParam;
	Util::Print(*nzbUrl);

	CNzb* nzb = new CNzb;

	// download .nzb from HTTP/HTTPS first
	if(nzbUrl->Left(5) == _T("http:")) {
		// create a new empty CNzb
		nzb->name = *nzbUrl;
		nzb->status = kFetching;
		{ NEWZFLOW_LOCK;
			nzb->refCount++; // so it doesn't get deleted while we're still downloading
			CNewzflow::Instance()->nzbs.Add(nzb); // add it to the queue so it shows up in CNzbView
		}

		// now download the file
		CString outFilename;
		// TODO: move this to a separate thread, so we don't block the control thread!
		if(CNewzflow::Instance()->httpDownloader->Download(*nzbUrl, nzb->GetLocalPath(), outFilename, &nzb->done)) {
			// parse the NZB 
			if(nzb->CreateFromLocal()) {
				{ NEWZFLOW_LOCK;
					nzb->name = outFilename; // adjust the name
					// TODO: move these 3 lines to nzb.cpp (duplicated from CNzb::CreateFromPath)
					CString right = nzb->name.Right(4);
					if(!right.CompareNoCase(_T(".nzb")))
						nzb->name = nzb->name.Left(nzb->name.GetLength() - 4);
					AddNZB(nzb);
					nzb->refCount--;
				}
			} else {
				// error during parsing, so remove NZB from queue
				{ NEWZFLOW_LOCK;
					nzb->refCount--;
				}
				CNewzflow::Instance()->RemoveNzb(nzb);
			}
		} else {
			nzb->status = kError;
			CString s; s.Format(_T("ERROR: %s\n"), CNewzflow::Instance()->httpDownloader->GetLastError());
			Util::Print(s);
			// TODO: what to do when the http downloader returns an error? remove the nzb? try again later?
		}
	} else {
		if(nzb->CreateFromPath(*nzbUrl)) {
			AddNZB(nzb);
		} else {
			delete nzb;
		}
	}

	delete nzbUrl;
	return 0;
}

LRESULT CNewzflowThread::OnAddNZB(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CNzb* nzb = (CNzb*)wParam;

	int error = ERROR_SUCCESS;
	if(CNewzflow::Instance()->settings->GetDownloadDir().IsEmpty() || !nzb->SetPath(CNewzflow::Instance()->settings->GetDownloadDir(), NULL, &error))
		Util::PostMessageToMainWindow(Util::MSG_SAVE_NZB, (WPARAM)nzb, (LPARAM)error);
	{ NEWZFLOW_LOCK;
		// dupecheck NZB in array; if it came from a URL, it's already added
		bool dupe = false;
		for(size_t i = 0; i < CNewzflow::Instance()->nzbs.GetCount(); i++) {
			if(CNewzflow::Instance()->nzbs[i] == nzb) {
				dupe = true;
				break;
			}
		}
		if(!dupe)
			CNewzflow::Instance()->nzbs.Add(nzb);

	}
	CNewzflow::Instance()->CreateDownloaders();
	WriteQueue();

	return 0;
}

LRESULT CNewzflowThread::OnWriteQueue(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CNewzflow::Instance()->WriteQueue();
	return 0;	
}

LRESULT CNewzflowThread::OnCreateDownloaders(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CNewzflow::Instance()->CreateDownloaders();
	return 0;	
}

class CQuitDialog : public CDialogImplEx<CQuitDialog>
{
public:
	enum { IDD = IDD_QUIT };

	BEGIN_MSG_MAP(CQuitDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		text = GetDlgItem(IDC_TEXT);
		progress = GetDlgItem(IDC_PROGRESS);
		progress.SetWindowLong(GWL_STYLE, progress.GetWindowLong(GWL_STYLE) | PBS_MARQUEE); // doesn't work when set directly from resource editor
		progress.SetMarquee(TRUE);
		return TRUE;
	}

public:
	CStatic text;
	CProgressBarCtrl progress;
};

/*static*/ CNewzflow* CNewzflow::s_pInstance = NULL;

// CNewzflow
//////////////////////////////////////////////////////////////////////////

CNewzflow::CNewzflow()
{
	ASSERT(s_pInstance == NULL);
	s_pInstance = this;

	VERIFY(CNntpSocket::InitWinsock());

	paused = false;
	pauseEnd = std::numeric_limits<time_t>::max();
	shutdownMode = Util::shutdown_nothing;
	shuttingDown = false;
	diskWriter = new CDiskWriter;
	postProcessor = new CPostProcessor;
	controlThread = new CNewzflowThread;
	settings = new CSettings;
	httpDownloader = new CHttpDownloader;
	if(!httpDownloader->Init()) {
		delete httpDownloader;
		httpDownloader = NULL;
	}
	// no downloaders have been created so far, so we don't need to propagate the speed limit to downloaders
	CNntpSocket::speedLimiter.SetLimit(settings->GetSpeedLimit());
	tvdbApi = new TheTvDB::CAPI;

	database.Open(settings->GetAppDataDir() + _T("database.dat"));
	ASSERT(database.IsOpen());
	{ sq3::Transaction transaction(database);
		database.Execute("CREATE TABLE IF NOT EXISTS \"RssFeeds\" (\"title\" TEXT, \"url\" TEXT NOT NULL UNIQUE, \"update_interval\" INTEGER DEFAULT 15, \"last_update\" REAL)");
		database.Execute("CREATE TABLE IF NOT EXISTS \"RssItems\" (\"feed\" INTEGER, \"title\" TEXT NOT NULL, \"link\" TEXT NOT NULL, \"length\" INTEGER DEFAULT 0, \"description\" TEXT, \"category\" TEXT, \"status\" INTEGER, \"date\" REAL, UNIQUE (feed, title))");
		database.Execute("CREATE TABLE IF NOT EXISTS \"TvShows\" (\"title\" TEXT, \"tvdb_id\" INTEGER, \"last_update\" REAL, \"description\" TEXT)");
		database.Execute("CREATE TABLE IF NOT EXISTS \"TvEpisodes\" (\"show_id\" INTEGER, \"tvdb_id\" INTEGER, \"title\" TEXT, \"season\" INTEGER, \"episode\" INTEGER, \"description\" TEXT, \"date\" REAL)");

		transaction.Commit();
	}

	dirWatcher = new CDirWatcher; // create after speed limiter intialization
	rssWatcher = new CRssWatcher;
}

CNewzflow::~CNewzflow()
{
	shuttingDown = true;

	CQuitDialog dlg;
	dlg.Create(NULL);

	dlg.text.SetWindowText(_T("Waiting for control thread..."));
	Util::Print("waiting for control thread to finish...\n");
	controlThread->PostQuitMessage();
	controlThread->JoinWithMessageLoop();
	delete controlThread;
	delete httpDownloader; // httpDownloader is only used from controlThread, so we can delete it after controlThread has been deleted

	dlg.text.SetWindowText(_T("Waiting for downloads..."));
	Util::Print("waiting for downloaders to finish...\n");
	// signal all downloaders' shutDown event
	for(size_t i = 0; i < downloaders.GetCount(); i++) {
		downloaders[i]->shutDown.Set();
	}
	CNntpSocket::CloseAllSockets(); // close all downloader sockets for immediate shutdown
	for(size_t i = 0; i < downloaders.GetCount(); i++) {
		downloaders[i]->JoinWithMessageLoop();
		delete downloaders[i];
	}
	downloaders.RemoveAll();

	for(size_t i = 0; i < finishedDownloaders.GetCount(); i++) {
		delete finishedDownloaders[i];
	}
	finishedDownloaders.RemoveAll();

	dlg.text.SetWindowText(_T("Waiting for disk writer..."));
	Util::Print("waiting for disk writer to finish...\n");
	diskWriter->PostQuitMessage();
	diskWriter->JoinWithMessageLoop();
	delete diskWriter;

	dlg.text.SetWindowText(_T("Waiting for post processor..."));
	Util::Print("waiting for post processor to finish...\n");
	postProcessor->PostQuitMessage();
	while(postProcessor->JoinWithMessageLoop(500) != WAIT_OBJECT_0) {
		CNzb* postProcNzb = (CNzb*)postProcessor->currentNzb;
		if(postProcNzb) {
			CString s;
			s.Format(_T("Waiting for post processor: %s..."), GetNzbStatusString(postProcNzb->status, postProcNzb->postProcStatus, postProcNzb->done));
			dlg.text.SetWindowText(s);
		}
	}
	delete postProcessor;

	dlg.text.SetWindowText(_T("Waiting for dir watcher..."));
	Util::Print("waiting for dir watcher to finish...\n");
	dirWatcher->shutDown.Set();
	dirWatcher->JoinWithMessageLoop();
	delete dirWatcher;

	dlg.text.SetWindowText(_T("Waiting for RSS watcher..."));
	Util::Print("waiting for RSS watcher to finish...\n");
	rssWatcher->shutDown.Set();
	rssWatcher->JoinWithMessageLoop();
	delete rssWatcher;

	WriteQueue();

	// delete NZBs
	for(size_t i = 0; i < nzbs.GetCount(); i++) {
		delete nzbs[i];
	}
	nzbs.RemoveAll();
	// delete deleted NZBs
	FreeDeletedNzbs();

	delete tvdbApi;
	delete settings;

	CNntpSocket::CloseWinsock();

	dlg.DestroyWindow();

	s_pInstance = NULL;
	Util::Print("~CNewzflow finished\n");
}

CNewzflow* CNewzflow::Instance()
{
	return s_pInstance;
}

// Downloaders get next segment to download
// to check if there's a segment, set bTestOnly = true, but don't do anything with the returning pointer except checking for NULL
CNzbSegment* CNewzflow::GetSegment(bool bTestOnly /*= false*/)
{
	NEWZFLOW_LOCK;
	if(shuttingDown)
		return NULL;

	for(size_t i = 0; i < nzbs.GetCount(); i++) {
		CNzb* nzb = nzbs[i];
		if(nzb->status == kPaused)
			continue;
		for(size_t j = 0; j < nzb->files.GetCount(); j++) {
			CNzbFile* file = nzb->files[j];
			if(file->status != kQueued)
				continue;
			for(size_t k = 0; k < file->segments.GetCount(); k++) {
				CNzbSegment* segment = file->segments[k];
				if(segment->status != kQueued)
					continue;
				if(!bTestOnly) {
					segment->status = kDownloading;
					nzb->refCount++;
				}
				return segment;
			}
		}
	}
	return NULL;
}

bool CNewzflow::HasSegment()
{
	return GetSegment(true) != NULL;
}

// Downloader is finished with a segment
void CNewzflow::UpdateSegment(CNzbSegment* s, ENzbStatus newStatus)
{
	// kError: a permanent error has occured (article not found)
	// kCached: segment has been successfully downloaded
	// kQueued: a temporary error has occured in the downloader. Segment needs to be tried again later
	ASSERT(newStatus == kError || newStatus == kCached || newStatus == kQueued);

	NEWZFLOW_LOCK;
	s->status = newStatus;
	CNzbFile* file = s->parent;
	CNzb* nzb = file->parent;
	nzb->refCount--;
	TRACE(_T("UpdateSegment(%s, %s): refCount=%d\n"), s->msgId, GetNzbStatusString(newStatus), nzb->refCount);
	CNewzflow::Instance()->diskWriter->Add(file);
}

// DiskWriter is finished joining a file
void CNewzflow::UpdateFile(CNzbFile* file, ENzbStatus newStatus)
{
	ASSERT(newStatus == kCompleted || newStatus == kError);

	NEWZFLOW_LOCK;
	CNzb* nzb = file->parent;
	nzb->refCount--;
	TRACE(_T("UpdateFile(%s, %s): refCount=%d\n"), file->fileName, GetNzbStatusString(newStatus), nzb->refCount);
	file->status = newStatus;
	for(size_t i = 0; i < nzb->files.GetCount(); i++) {
		CNzbFile* f = nzb->files[i];
		if(f->status != kCompleted && f->status != kError && f->status != kPaused)
			return;
	}
	// if all files of the NZB are either completed, paused (PAR files), or errored, tell the post processor to start validating/repairing/unpacking
	nzb->status = kCompleted;
	postProcessor->Add(nzb);
}

// a finished downloader calls this to remove itself from the downloaders array
// it's put into the finishedDownloaders array that will be cleaned upon AddJob and ~CNewzflow
// unfortunately a thread cannot delete itself.
void CNewzflow::RemoveDownloader(CDownloader* dl)
{
	NEWZFLOW_LOCK;

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

	// if all downloaders are removed, but there are still segments left, start new downloaders (from controlThread)
	if(downloaders.GetCount() == 0 && HasSegment())
		CNewzflow::Instance()->controlThread->CreateDownloaders();
}

// There's a race condition where downloader's currently shutting down
// but not yet removed from downloaders array, resulting in no new downloaders being created
// To avoid this, RemoveDownloader() now checks the queue and calls CreateDownloaders() from controlThread if necessary
void CNewzflow::CreateDownloaders()
{
	NEWZFLOW_LOCK;

	// delete any finished downloaders (they can't delete themselves)
	for(size_t i = 0; i < finishedDownloaders.GetCount(); i++) {
		delete finishedDownloaders[i];
	}
	finishedDownloaders.RemoveAll();

	if(paused || !HasSegment())
		return;

	int maxDownloaders = min(100, max(0, _ttoi(settings->GetIni(_T("Server"), _T("Connections"), _T("10")))));

	// create enough downloaders
	int numDownloaders = max<int>(0, maxDownloaders - downloaders.GetCount());

	CString s;
	s.Format(_T("CreateDownloaders(): Have %d, need %d, creating %d.\n"), downloaders.GetCount(), maxDownloaders, numDownloaders);
	Util::Print(s);

	for(int i = 0; i < numDownloaders; i++) {
		downloaders.Add(new CDownloader);
	}
}

// called by settings dialog when server settings have changed
void CNewzflow::OnServerSettingsChanged()
{
	NEWZFLOW_LOCK;

	// set new speed limit; don't need to propagate to downloaders, because they will be recreated anyway
	CNntpSocket::speedLimiter.SetLimit(settings->GetSpeedLimit());

	// shut down all downloaders - hostname/port/username/password could have been changed, so we need to reconnect
	for(size_t i = 0; i < downloaders.GetCount(); i++) {
		downloaders[i]->shutDown.Set();
	}
	Util::Print("Finished shutting down downloaders\n");

	// when downloaders are shutting down, CreateDownloaders will be called to recreate new downloaders
}

// write the queue to disk so it can be restored later
// called from CNewzflowThread
void CNewzflow::WriteQueue()
{
	CMemFile mf(128*1024, 128*1024);
	{ NEWZFLOW_LOCK;
		size_t nzbCount = nzbs.GetCount();
		mf.Write<unsigned>('NFQ!'); // ID
		mf.Write<unsigned>(2); // Version
		mf.Write<size_t>(nzbCount);
		for(size_t i = 0; i < nzbCount; i++) {
			CNzb* nzb = nzbs[i];
			mf.Write<GUID>(nzb->guid);
			mf.WriteString(nzb->name);
			mf.WriteString(nzb->path);
			mf.Write<char>(nzb->doRepair);
			mf.Write<char>(nzb->doUnpack);
			mf.Write<char>(nzb->doDelete);
			mf.Write<char>(nzb->status);
			size_t fileCount = nzb->files.GetCount();
			mf.Write<size_t>(fileCount);
			for(size_t j = 0; j < fileCount; j++) {
				CNzbFile* file = nzb->files[j];
				mf.Write<char>(file->status);
				mf.Write<char>(file->parStatus);
				mf.Write<MD5_CTX>(file->md5);
				size_t segCount = file->segments.GetCount();
				mf.Write<size_t>(segCount);
				for(size_t k = 0; k < segCount; k++) {
					CNzbSegment* seg = file->segments[k];
					mf.Write<char>(seg->status);
				}
			}
			size_t parCount = nzb->parSets.GetCount();
			mf.Write<size_t>(parCount);
			for(size_t j = 0; j < parCount; j++) {
				CParSet* parSet = nzb->parSets[j];
				mf.Write<char>(parSet->completed);
			}
			if(!nzb->log.IsEmpty()) {
				CTextFile tf;
				if(tf.Open(nzb->GetLogPath(), CTextFile::kWrite))
					tf.Write(nzb->log);
			} else {
				CFile::Delete(nzb->GetLogPath());
			}
		}
	}
	CFile f;
	DeleteFile(settings->GetAppDataDir() + _T("queue.dat"));
	if(f.Open(settings->GetAppDataDir() + _T("queue.dat"), GENERIC_WRITE, 0, CREATE_ALWAYS)) {
		f.Write(mf.GetBuffer(), mf.GetSize());
		f.Close();
	}
}

// returns true if the queue contains any NZBs that are queued/downloading
bool CNewzflow::ReadQueue()
{
	bool ret = false;

	CFile f;
	if(f.Open(settings->GetAppDataDir() + _T("queue.dat"), GENERIC_READ, 0, OPEN_EXISTING)) {
		size_t size = (size_t)f.GetSize();
		void* buffer = new char [size];
		f.Read(buffer, size);
		f.Close();
		
		CMemFile mf(buffer, size);
		if(mf.Read<unsigned>() != 'NFQ!') {
			return false;
		}
		unsigned version = mf.Read<unsigned>();
		if(version != 2) {
			return false;
		}
		CAtlArray<CNzb*> newNzbs;
		size_t nzbCount = mf.Read<size_t>();
		for(size_t i = 0; i < nzbCount; i++) {
			GUID guid = mf.Read<GUID>();
			CString name = mf.ReadString();
			CNzb* nzb = new CNzb;
			if(nzb->CreateFromQueue(guid, name)) {
				nzb->path = mf.ReadString();
				nzb->doRepair = !!mf.Read<char>();
				nzb->doUnpack = !!mf.Read<char>();
				nzb->doDelete = !!mf.Read<char>();
				nzb->status = (ENzbStatus)mf.Read<char>();
				size_t fileCount = mf.Read<size_t>();
				ASSERT(fileCount == nzb->files.GetCount());
				for(size_t j = 0; j < fileCount; j++) {
					CNzbFile* file = nzb->files[j];
					file->status = (ENzbStatus)mf.Read<char>();
					file->parStatus = (EParStatus)mf.Read<char>();
					file->md5 = mf.Read<MD5_CTX>();
					size_t segCount = mf.Read<size_t>();
					ASSERT(segCount == file->segments.GetCount());
					for(size_t k = 0; k < segCount; k++) {
						CNzbSegment* seg = file->segments[k];
						seg->status = (ENzbStatus)mf.Read<char>();
						// we can't continue from downloading status, so need to requeue segment
						if(seg->status == kDownloading)
							seg->status = kQueued;
					}
					if(file->status == kQueued) // there's a queued file, downloaders need to be created
						ret = true;
				}
				size_t parCount = mf.Read<size_t>();
				ASSERT(parCount == nzb->parSets.GetCount());
				for(size_t j = 0; j < parCount; j++) {
					CParSet* parSet = nzb->parSets[j];
					parSet->completed = !!mf.Read<char>();
				}
				newNzbs.Add(nzb);
				CTextFile tf;
				if(tf.Open(nzb->GetLogPath(), CTextFile::kRead))
					tf.Read(nzb->log);
			} else { // NZB-file in %appdir% doesn't exist, so we need to skip all the data for this NZB in the queue file
				delete nzb;
				mf.ReadString(); // path
				mf.Read<char>(); // doRepair
				mf.Read<char>(); // doUnpack
				mf.Read<char>(); // doDelete
				mf.Read<char>(); // status
				size_t fileCount = mf.Read<size_t>();
				for(size_t j = 0; j < fileCount; j++) {
					mf.Read<char>(); // file->status
					mf.Read<char>(); // file->parStatus
					mf.Read<MD5_CTX>(); // file->md5
					size_t segCount = mf.Read<size_t>();
					for(size_t k = 0; k < segCount; k++) {
						mf.Read<char>();
					}
				}
				size_t parCount = mf.Read<size_t>();
				for(size_t j = 0; j < parCount; j++) {
					mf.Read<char>();
				}
			}
		}
		{ NEWZFLOW_LOCK;
			nzbs.Append(newNzbs);
			// if there are NZBs that still don't have a path (user closed the app when "Save As..." dialog was shown), show the "Save As..." dialog again
			for(size_t i = 0; i < newNzbs.GetCount(); i++) {
				if(newNzbs[i]->path.IsEmpty())
					Util::PostMessageToMainWindow(Util::MSG_SAVE_NZB, (WPARAM)newNzbs[i]);
			}
		}
	}
	return ret;
}

// called from main thread (CMainFrame::OnNzbRemove) or CNewzflowThread
void CNewzflow::RemoveNzb(CNzb* nzb)
{
	NEWZFLOW_LOCK; // CMainframe already locked, but it's the same thread, so no deadlock here
	for(size_t i = 0; i < nzbs.GetCount(); i++) {
		if(nzbs[i] == nzb) {
			nzbs.RemoveAt(i);
			// if NZB's refCount is 0, we can delete it immediately
			// otherwise there are still downloaders or the post processor active,
			// so just move it to a different array where we will periodically check whether we can delete it for good
			if(nzb->refCount == 0) {
				nzb->Cleanup();
				delete nzb;
			} else {
				TRACE(_T("Can't remove %s yet. refCount=%d\n"), nzb->name, nzb->refCount);
				deletedNzbs.Add(nzb);
			}
			break;
		}
	}
}

void CNewzflow::FreeDeletedNzbs()
{
	// check if we can remove some of the deleted NZBs
	NEWZFLOW_LOCK;
	for(size_t i = 0; i < deletedNzbs.GetCount(); i++) {
		if(deletedNzbs[i]->refCount == 0) {
			TRACE(_T("Finally can remove %s.\n"), deletedNzbs[i]->name);
			deletedNzbs[i]->Cleanup();
			delete deletedNzbs[i];
			deletedNzbs.RemoveAt(i);
			i--;
		}
	}
}

bool CNewzflow::IsShuttingDown()
{
	return shuttingDown;
}

void CNewzflow::SetSpeedLimit(int limit)
{
	NEWZFLOW_LOCK;
	CNntpSocket::speedLimiter.SetLimit(limit);
	CNewzflow::Instance()->settings->SetSpeedLimit(limit);

	// propagate speed limit to all downloaders (they need to adjust their socket rcvbuf size)
	for(size_t i = 0; i < downloaders.GetCount(); i++) {
		downloaders[i]->sock.SetLimit();
	}
}

void CNewzflow::AddPostProcessor(CNzb* nzb)
{
	postProcessor->Add(nzb);
}

bool CNewzflow::IsPaused()
{
	return paused;
}

void CNewzflow::Pause(bool pause, int length)
{
	paused = pause;
	pauseEnd = std::numeric_limits<time_t>::max();
	if(pause && length != INT_MAX)
		pauseEnd = time(NULL) + (time_t)length;

	if(pause) {
		NEWZFLOW_LOCK;

		// shut down all downloaders; TODO: move somewhere else
		for(size_t i = 0; i < downloaders.GetCount(); i++) {
			downloaders[i]->shutDown.Set();
		}
		Util::Print("Finished shutting down downloaders\n");
	} else {
		controlThread->CreateDownloaders();
	}
}

CString CNewzflow::GetPauseStatus()
{
	if(paused) {
		if(pauseEnd == std::numeric_limits<time_t>::max()) {
			return _T("Paused");
		} else {
			return CString(_T("Paused for ")) + Util::FormatTimeSpan(pauseEnd - time(NULL));
		}
	} else {
		if(IsDownloading())
			return _T("Downloading");
		else
			return _T("Idle");
	}
}

void CNewzflow::OnTimer()
{
	if(paused && time(NULL) >= pauseEnd) {
		Pause(false);
	}
}

bool CNewzflow::IsDownloading()
{
	bool bDownloading = false;
	{
		NEWZFLOW_LOCK;
		size_t count = nzbs.GetCount();
		for(size_t i = 0; i < count; i++) {
			CNzb* nzb = nzbs[i];
			for(size_t j = 0; j < nzb->files.GetCount(); j++) {
				CNzbFile* f = nzb->files[j];
				if(f->status == kPaused)
					continue;
				for(size_t k = 0; k < f->segments.GetCount(); k++) {
					CNzbSegment* s = f->segments[k];
					if(s->status == kPaused)
						continue;
					if(s->status == kDownloading)
						bDownloading = true;
				}
				if(bDownloading)
					break;
			}
			if(bDownloading)
				break;
		}
	}
	return bDownloading;
}

void CNewzflow::SetShutdownMode(Util::EShutdownMode mode)
{
	shutdownMode = mode;
}

Util::EShutdownMode CNewzflow::GetShutdownMode()
{
	return shutdownMode;
}

void CNewzflow::RefreshRssWatcher()
{
	rssWatcher->refresh.Set();
}

/*static*/ const char* CNewzflow::CLock::file = NULL;
/*static*/ int CNewzflow::CLock::line = NULL;
