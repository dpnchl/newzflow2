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

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// TODO:
// - investigate why downloads get corrupted during testing (forcibly shutting down Newzbin lots of times)
// - CNzbView/CFileView: implement progress bar with no theming
// - CNewzflowThread:: AddFile(): what to do when a file is added that is already in the queue. can it be detected? should it be skipped or renamed (add counter) and added anyway?
// - during shut down, it should be avoided to add jobs to the disk writer or post processor; have to check if queue states are correctly restored if we just skip adding jobs.

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

	PostThreadMessage(MSG_ADD_NZB, (WPARAM)new CString(nzbUrl));
}

void CNewzflowThread::AddURL(const CString& nzbUrl)
{
// currently not supported until HTTP(S) downloader is complete
// we want to avoid passing http:// urls to ISAXXMLReader, because it uses WinInet to receive the files. WinInet is known to cache files locally that shouldn't be
	PostThreadMessage(MSG_ADD_NZB, (WPARAM)new CString(nzbUrl));
}

void CNewzflowThread::WriteQueue()
{
	PostThreadMessage(MSG_WRITE_QUEUE);
}

void CNewzflowThread::CreateDownloaders()
{
	PostThreadMessage(MSG_CREATE_DOWNLOADERS);
}

LRESULT CNewzflowThread::OnAddNzb(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CString* nzbUrl = (CString*)wParam;

	CNzb* nzb = new CNzb;

	// download .nzb from HTTP/HTTPS first
	if(nzbUrl->Left(5) == _T("http:")) {
		// create a new empty CNzb
		nzb->name = *nzbUrl;
		nzb->status = kFetching;
		{ CNewzflow::CLock lock;
			nzb->refCount++; // so it doesn't get deleted while we're still downloading
			CNewzflow::Instance()->nzbs.Add(nzb); // add it to the queue so it shows up in CNzbView
		}

		// now download the file
		CString outFilename;
		// TODO: move this to a separate thread, so we don't block the control thread!
		if(CNewzflow::Instance()->httpDownloader->Download(*nzbUrl, nzb->GetLocalPath(), outFilename, &nzb->done)) {
			// parse the NZB 
			if(nzb->CreateFromLocal()) {
				{ CNewzflow::CLock lock;
					nzb->name = outFilename; // adjust the name
					nzb->refCount--;
				}
				CNewzflow::Instance()->CreateDownloaders();
				WriteQueue();
			} else {
				// error during parsing, so remove NZB from queue
				{ CNewzflow::CLock lock;
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
			{ CNewzflow::CLock lock;
				CNewzflow::Instance()->nzbs.Add(nzb);
			}
			CNewzflow::Instance()->CreateDownloaders();
			WriteQueue();
		} else {
			delete nzb;
		}
	}

	delete nzbUrl;
	return 0;
}

class CMemFile
{
public:
	CMemFile()
	{
		m_buffer = NULL;
		m_bufferFull = m_bufferSize = m_bufferGrow = 0;
	}
	CMemFile(int bufferSize, int bufferGrow) 
	{
		m_buffer = NULL;
		Create(bufferSize, bufferGrow);
	}
	CMemFile(void* buffer, int bufferSize)
	{
		m_buffer = NULL;
		Attach(buffer, bufferSize);
	}
	~CMemFile()
	{
		delete m_buffer;
	}
	void Create(int bufferSize, int bufferGrow) 
	{
		delete m_buffer;
		m_buffer = new char [bufferSize];
		m_bufferSize = bufferSize;
		m_bufferGrow = bufferGrow;
		m_bufferFull = 0;
	}
	void Attach(void* buffer, int bufferSize)
	{
		delete m_buffer;
		m_buffer = buffer;
		m_bufferSize = bufferSize;
		m_bufferFull = 0;
		m_bufferGrow = 0;
	}
	template <class T>
	void Write(const T& data)
	{
		WriteBinary(&data, sizeof(T));
	}
	void WriteString(const CString& str)
	{
		WriteBinary((const TCHAR*)str, str.GetLength() * sizeof(TCHAR));
		Write<TCHAR>(0);
	}
	void WriteBinary(const void* data, int size)
	{
		if(m_bufferSize - m_bufferFull < size)
			Grow();
		memcpy((char*)m_buffer + m_bufferFull, data, size);
		m_bufferFull += size;
	}
	template <class T>
	T Read()
	{
		T data;
		ReadBinary(&data, sizeof(T));
		return data;
	}
	CString ReadString()
	{
		TCHAR* str = (TCHAR*)((char*)m_buffer + m_bufferFull);
		int start = m_bufferFull;
		while(*(TCHAR*)((char*)m_buffer + m_bufferFull) != 0 && m_bufferFull < m_bufferSize) {
			m_bufferFull += sizeof(TCHAR);
		}
		if(*(TCHAR*)((char*)m_buffer + m_bufferFull) == 0) {
			m_bufferFull += sizeof(TCHAR);
			return CString(str, (m_bufferFull - start) / sizeof(TCHAR) - 1);
		} else
			return _T(""); // error
	}
	void ReadBinary(void* data, int size)
	{
		memcpy(data, (char*)m_buffer + m_bufferFull, min(size, m_bufferSize - m_bufferFull));
		m_bufferFull += size;
	}
	void* GetBuffer()
	{
		return m_buffer;
	}
	int GetSize()
	{
		return m_bufferFull;
	}

protected:
	void Grow() 
	{
		ASSERT(m_buffer);
		ASSERT(m_bufferGrow > 0);
		m_bufferSize += m_bufferGrow;
		void* newBuffer = new char [m_bufferSize];
		memcpy(newBuffer, m_buffer, m_bufferFull);
		delete m_buffer;
		m_buffer = newBuffer;
	}

protected:
	void* m_buffer;
	int m_bufferFull;
	int m_bufferSize;
	int m_bufferGrow;
};

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

class CShutdownDialog : public CDialogImpl<CShutdownDialog>
{
public:
	enum { IDD = IDD_SHUTDOWN };

	BEGIN_MSG_MAP(CShutdownDialog)
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
}

CNewzflow::~CNewzflow()
{
	shuttingDown = true;

	CShutdownDialog dlg;
	dlg.Create(NULL);

	dlg.text.SetWindowText(_T("Waiting for control thread..."));
	Util::Print("waiting for control thread to finish...\n");
	controlThread->PostQuitMessage();
	controlThread->JoinWithMessageLoop();
	delete controlThread;
	delete httpDownloader; // httpDownloader is only used from controlThread, so we can delete it after controlThread has been deleted

	dlg.text.SetWindowText(_T("Waiting for downloads..."));
	Util::Print("waiting for downloaders to finish...\n");
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
			s.Format(_T("Waiting for post processor: %s..."), GetNzbStatusString(postProcNzb->status, postProcNzb->done));
			dlg.text.SetWindowText(s);
		}
	}
	delete postProcessor;

	WriteQueue();

	// delete NZBs
	for(size_t i = 0; i < nzbs.GetCount(); i++) {
		delete nzbs[i];
	}
	nzbs.RemoveAll();
	// delete deleted NZBs
	FreeDeletedNzbs();

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

bool CNewzflow::HasSegment()
{
	CLock lock;
	if(shuttingDown)
		return false;

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
				return true;
			}
		}
	}
	return false;
}

// Downloader/DiskWriter is finished with a segment
void CNewzflow::UpdateSegment(CNzbSegment* s, ENzbStatus newStatus)
{
	// kError: a permanent error has occured (article not found)
	// kCached: segment has been successfully downloaded
	// kQueued: a temporary error has occured in the downloader. Segment needs to be tried again later
	ASSERT(newStatus == kError || newStatus == kCached || newStatus == kQueued);

	CLock lock;
	s->status = newStatus;
	CNzbFile* file = s->parent;
	CNzb* nzb = file->parent;
	nzb->refCount--;
	TRACE(_T("UpdateSegment(%s, %s): refCount=%d\n"), s->msgId, GetNzbStatusString(newStatus), nzb->refCount);
	// check if all segments of the file are cached or error / completed or error
	bool bCached = true;
	bool bCompleted = true;
	for(size_t i = 0; i < file->segments.GetCount(); i++) {
		CNzbSegment* segment = file->segments[i];
		if(segment->status != kCached && segment->status != kError)
			bCached = false;
		if(segment->status != kCompleted && segment->status != kError)
			bCompleted = false;
	}
	// if all segments of a file are either cached or errored, tell the disk writer to join them together
	if(bCached) {
		file->status = kCached;
		CNewzflow::Instance()->diskWriter->Add(file);
		return;
	}
}

// DiskWriter is finished joining a file
void CNewzflow::UpdateFile(CNzbFile* file, ENzbStatus newStatus)
{
	ASSERT(newStatus == kCompleted);

	CLock lock;
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

	// if all downloaders are removed, but there are still segments left, start new downloaders (from controlThread)
	if(downloaders.GetCount() == 0 && HasSegment())
		CNewzflow::Instance()->controlThread->CreateDownloaders();
}

// There's a race condition where downloader's currently shutting down
// but not yet removed from downloaders array, resulting in no new downloaders being created
// To avoid this, RemoveDownloader() now checks the queue and calls CreateDownloaders() from controlThread if necessary
void CNewzflow::CreateDownloaders()
{
	CLock lock;

	// delete any finished downloaders (they can't delete themselves)
	for(size_t i = 0; i < finishedDownloaders.GetCount(); i++) {
		delete finishedDownloaders[i];
	}
	finishedDownloaders.RemoveAll();

	if(!HasSegment())
		return;

	int maxDownloaders = min(100, max(0, _ttoi(settings->GetIni(_T("Server"), _T("Connections"), _T("10")))));

	// create enough downloaders
	int numDownloaders = max(0, maxDownloaders - downloaders.GetCount());

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
	CLock lock;

	// set new speed limit; don't need to propagate to downloaders, because they will be recreated anyway
	CNntpSocket::speedLimiter.SetLimit(settings->GetSpeedLimit());

	// shut down all downloaders - hostname/port/username/password could have been changed, so we need to reconnect
	for(size_t i = 0; i < downloaders.GetCount(); i++) {
		CString s;
		downloaders[i]->shutDown = true;
	}
	Util::Print("Finished shutting down downloaders\n");

	// when downloaders are shutting down, CreateDownloaders will be called to recreate new downloaders
}


// write the queue to disk so it can be restored later
// called from CNewzflowThread
void CNewzflow::WriteQueue()
{
	CMemFile mf(128*1024, 128*1024);
	{ CNewzflow::CLock lock;
		size_t nzbCount = nzbs.GetCount();
		mf.Write<unsigned>('NFQ!'); // ID
		mf.Write<unsigned>(1); // Version
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
		if(version != 1) {
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
				if(nzb->status == kDownloading)
					nzb->status = kQueued;
				size_t fileCount = mf.Read<size_t>();
				ASSERT(fileCount == nzb->files.GetCount());
				for(size_t j = 0; j < fileCount; j++) {
					CNzbFile* file = nzb->files[j];
					file->status = (ENzbStatus)mf.Read<char>();
					size_t segCount = mf.Read<size_t>();
					ASSERT(segCount == file->segments.GetCount());
					for(size_t k = 0; k < segCount; k++) {
						CNzbSegment* seg = file->segments[k];
						seg->status = (ENzbStatus)mf.Read<char>();
						// we can't continue from downloading status, so need to requeue segment
						if(seg->status == kDownloading)
							seg->status = kQueued;
					}
				}
				size_t parCount = mf.Read<size_t>();
				ASSERT(parCount == nzb->parSets.GetCount());
				for(size_t j = 0; j < parCount; j++) {
					CParSet* parSet = nzb->parSets[j];
					parSet->completed = !!mf.Read<char>();
				}
				newNzbs.Add(nzb);
				if(nzb->status == kQueued) // if NZB hasn't finished downloading, downloaders need to be created
					ret = true;
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
		{ CLock lock;
			nzbs.Append(newNzbs);
		}
	}
	return ret;
}

// called from main thread (CMainFrame::OnNzbRemove) or CNewzflowThread
void CNewzflow::RemoveNzb(CNzb* nzb)
{
	CLock lock; // CMainframe already locked, but it's the same thread, so no deadlock here
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
	CLock lock;
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
	CLock lock;
	CNntpSocket::speedLimiter.SetLimit(limit);
	CNewzflow::Instance()->settings->SetSpeedLimit(limit);

	// propagate speed limit to all downloaders (they need to adjust their socket rcvbuf size)
	for(size_t i = 0; i < downloaders.GetCount(); i++) {
		downloaders[i]->sock.SetLimit();
	}
}