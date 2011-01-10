#pragma once

#include "nzb.h"
#include "Util.h"

class CDownloader;
class CDiskWriter;
class CPostProcessor;
class CSettings;
class CHttpDownloader;
class CDirWatcher;
class CRssWatcher;
class CDatabase;
namespace TheTvDB { class CAPI; }
namespace TheMovieDB { class CAPI; }

class CNewzflowThread : public CGuiThreadImpl<CNewzflowThread>
{
	enum {
		MSG_ADD_FILE = WM_USER+1,	// wParam = (CString*)nzbFileName
		MSG_ADD_NZB,				// wParam = (CNzb*)nzb
		MSG_WRITE_QUEUE,
		MSG_CREATE_DOWNLOADERS,
	};

	BEGIN_MSG_MAP(CNewzflowThread)
		MESSAGE_HANDLER(MSG_ADD_FILE, OnAddFile)
		MESSAGE_HANDLER(MSG_ADD_FILE, OnAddFile)
		MESSAGE_HANDLER(MSG_ADD_NZB, OnAddNZB)
		MESSAGE_HANDLER(MSG_WRITE_QUEUE, OnWriteQueue)
		MESSAGE_HANDLER(MSG_CREATE_DOWNLOADERS, OnCreateDownloaders)
	END_MSG_MAP()

public:
	CNewzflowThread() : CGuiThreadImpl<CNewzflowThread>(&_Module, CREATE_SUSPENDED) { Resume(); }

	void AddFile(const CString& nzbUrl);
	void AddNZB(CNzb* nzb);
	void AddURL(const CString& nzbUrl);
	void WriteQueue();
	void CreateDownloaders();

protected:
	LRESULT OnAddFile(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnAddNZB(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnWriteQueue(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCreateDownloaders(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

class CNewzflow
{
public:
	#ifdef _DEBUG
		#define NEWZFLOW_LOCK CNewzflow::CLock lock(__FILE__, __LINE__)
	#else
		#define NEWZFLOW_LOCK CNewzflow::CLock lock
	#endif
	class CLock : public CComCritSecLock<CComAutoCriticalSection> {
	public:
		CLock(const char* _file = NULL, int _line = 0) : CComCritSecLock<CComAutoCriticalSection>(CNewzflow::Instance()->cs) 
		{
			file = _file;
			line = _line;
		}
		static const char* file;
		static int line;
	};

	CNewzflow();
	~CNewzflow();
	static CNewzflow* Instance();

	bool HasSegment();
	CNzbSegment* GetSegment(bool bTestOnly = false);
	bool IsShuttingDown();
	bool IsPaused();
	void Pause(bool pause, int length = INT_MAX);
	CString GetPauseStatus();

	bool IsDownloading();
	void SetShutdownMode(Util::EShutdownMode mode);
	Util::EShutdownMode GetShutdownMode();
	void UpdateSegment(CNzbSegment* s, ENzbStatus newStatus);
	void UpdateFile(CNzbFile* f, ENzbStatus newStatus);
	void RemoveDownloader(CDownloader* dl);
	void CreateDownloaders();
	void WriteQueue();
	bool ReadQueue();
	void RemoveNzb(CNzb* nzb);
	void FreeDeletedNzbs();
	void OnServerSettingsChanged();
	void OnProviderSettingsChanged();
	void OnTimer();
	void SetSpeedLimit(int limit);
	void AddPostProcessor(CNzb* nzb);
	void RefreshRssWatcher();
	CAtlArray<CNzb*> nzbs, deletedNzbs;

	CAtlArray<CDownloader*> downloaders, finishedDownloaders;
	CDiskWriter* diskWriter;
	CPostProcessor* postProcessor;
	CNewzflowThread* controlThread;
	CSettings* settings;
	CHttpDownloader* httpDownloader;
	CDirWatcher* dirWatcher;
	CRssWatcher* rssWatcher;
	TheTvDB::CAPI* tvdbApi;
	TheMovieDB::CAPI* tmdbApi;
	CDatabase* database;

	CComAutoCriticalSection cs;

protected:
	static CNewzflow* s_pInstance;

	bool paused;
	time_t pauseEnd;
	Util::EShutdownMode shutdownMode;

	volatile bool shuttingDown;
};
