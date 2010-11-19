#pragma once

#include "nzb.h"

class CDownloader;
class CDiskWriter;
class CPostProcessor;
class CSettings;
class CHttpDownloader;

class CNewzflowThread : public CGuiThreadImpl<CNewzflowThread>
{
	enum {
		MSG_ADD_NZB = WM_USER+1,
		MSG_WRITE_QUEUE,
	};

	BEGIN_MSG_MAP(CNewzflowThread)
		MESSAGE_HANDLER(MSG_ADD_NZB, OnAddNzb)
		MESSAGE_HANDLER(MSG_WRITE_QUEUE, OnWriteQueue)
	END_MSG_MAP()

public:
	CNewzflowThread() : CGuiThreadImpl<CNewzflowThread>(&_Module) {}

	void AddFile(const CString& nzbUrl);
	void AddURL(const CString& nzbUrl);
	void WriteQueue();

protected:
	LRESULT OnAddNzb(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	LRESULT OnWriteQueue(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

class CNewzflow
{
public:
	class CLock : public CComCritSecLock<CComAutoCriticalSection> {
	public:
		CLock() : CComCritSecLock<CComAutoCriticalSection>(CNewzflow::Instance()->cs) {}
	};

	CNewzflow();
	~CNewzflow();
	static CNewzflow* Instance();

	CNzbSegment* GetSegment();
	bool IsShuttingDown();
	void UpdateSegment(CNzbSegment* s, ENzbStatus newStatus);
	void UpdateFile(CNzbFile* f, ENzbStatus newStatus);
	void RemoveDownloader(CDownloader* dl);
	void CreateDownloaders();
	void WriteQueue();
	bool ReadQueue();
	void RemoveNzb(CNzb* nzb);
	void FreeDeletedNzbs();
	CAtlArray<CNzb*> nzbs, deletedNzbs;
	CAtlArray<CDownloader*> downloaders, finishedDownloaders;
	CDiskWriter* diskWriter;
	CPostProcessor* postProcessor;
	CNewzflowThread* controlThread;
	CSettings* settings;
	CHttpDownloader* httpDownloader;

	CComAutoCriticalSection cs;

protected:
	static CNewzflow* s_pInstance;

	volatile bool shuttingDown;
};
