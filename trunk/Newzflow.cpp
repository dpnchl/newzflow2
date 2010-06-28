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

	PostThreadMessage(MSG_ADD_NZB, (WPARAM)new CString(nzbUrl));
}

void CNewzflowThread::AddURL(const CString& nzbUrl)
{
// currently not supported until HTTP(S) downloader is complete
// we want to avoid passing http:// urls to ISAXXMLReader, because it uses WinInet to receive the files. WinInet is known to cache files locally that shouldn't be
//	PostThreadMessage(MSG_JOB, (WPARAM)new CString(nzbUrl));
}

void CNewzflowThread::WriteQueue()
{
	PostThreadMessage(MSG_WRITE_QUEUE);
}

LRESULT CNewzflowThread::OnAddNzb(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CString* nzbUrl = (CString*)wParam;

	CNzb* nzb = CNzb::Create(*nzbUrl);
	if(nzb) {
		{ CNewzflow::CLock lock;
			CNewzflow::Instance()->nzbs.Add(nzb);
		}
		CNewzflow::Instance()->CreateDownloaders();
		WriteQueue();
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
	ASSERT(newStatus == kError || newStatus == kCompleted || newStatus == kCached);

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

void CNewzflow::CreateDownloaders()
{
	CLock lock;

	// delete any finished downloaders (they can't delete themselves)
	for(size_t i = 0; i < finishedDownloaders.GetCount(); i++) {
		delete finishedDownloaders[i];
	}
	finishedDownloaders.RemoveAll();

	if(nzbs.IsEmpty())
		return;

	// create enough downloaders
	int numDownloaders = max(0, 5 - downloaders.GetCount());
	for(int i = 0; i < numDownloaders; i++) {
		downloaders.Add(new CDownloader);
	}
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
		}
	}
	CFile f;
	DeleteFile(settings->GetAppDataDir() + _T("queue.dat"));
	if(f.Open(settings->GetAppDataDir() + _T("queue.dat"), GENERIC_WRITE, 0, CREATE_ALWAYS)) {
		f.Write(mf.GetBuffer(), mf.GetSize());
		f.Close();
	}
}

void CNewzflow::ReadQueue()
{
	CFile f;
	if(f.Open(settings->GetAppDataDir() + _T("queue.dat"), GENERIC_READ, 0, OPEN_EXISTING)) {
		size_t size = (size_t)f.GetSize();
		void* buffer = new char [size];
		f.Read(buffer, size);
		f.Close();
		
		CMemFile mf(buffer, size);
		if(mf.Read<unsigned>() != 'NFQ!') {
			return;
		}
		unsigned version = mf.Read<unsigned>();
		if(version != 1) {
			return;
		}
		CAtlArray<CNzb*> newNzbs;
		size_t nzbCount = mf.Read<size_t>();
		for(size_t i = 0; i < nzbCount; i++) {
			GUID guid = mf.Read<GUID>();
			CString name = mf.ReadString();
			CNzb* nzb = CNzb::Create(guid, name);
			if(nzb) {
				nzb->path = mf.ReadString();
				nzb->doRepair = !!mf.Read<char>();
				nzb->doUnpack = !!mf.Read<char>();
				nzb->doDelete = !!mf.Read<char>();
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
						// we can't continue from cached or downloading status, so need to requeue segment
						if(seg->status == kCached || seg->status == kDownloading)
							seg->status = kQueued;
					}
				}
				newNzbs.Add(nzb);
			} else { // NZB-file in %appdir% doesn't exist, so we need to skip all the data for this NZB in the queue file
				mf.ReadString(); // path
				mf.Read<char>(); // doRepair
				mf.Read<char>(); // doUnpack
				mf.Read<char>(); // doDelete
				size_t fileCount = mf.Read<size_t>();
				for(size_t j = 0; j < fileCount; j++) {
					mf.Read<char>(); // file->status
					size_t segCount = mf.Read<size_t>();
					for(size_t k = 0; k < segCount; k++) {
						mf.Read<char>();
					}
				}
			}
		}
		{ CLock lock;
			nzbs.Append(newNzbs);
		}
	}
}

// called from main thread (CMainFrame::OnNzbRemove)
void CNewzflow::RemoveNzb(CNzb* nzb)
{
	CLock lock; // CMainframe already locked, but it's the same thread, so no deadlock here
	for(size_t i = 0; i < nzbs.GetCount(); i++) {
		if(nzbs[i] == nzb) {
			nzbs.RemoveAt(i);
			DeleteFile(settings->GetAppDataDir() + CComBSTR(nzb->guid) + _T(".nzb")); // delete cached .nzb
			// if NZB's refCount is 0, we can delete it immediately
			// otherwise there are still downloaders or the post processor active,
			// so just move it to a different array where we will periodically check whether we can delete it for good
			if(nzb->refCount == 0)
				delete nzb;
			else {
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
			delete deletedNzbs[i];
			deletedNzbs.RemoveAt(i);
			i--;
		}
	}
}
