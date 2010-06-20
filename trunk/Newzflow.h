#pragma once

#include "nzb.h"

class CDownloader;
class CDiskWriter;
class CPostProcessor;

class CNewzflowThread : public CGuiThreadImpl<CNewzflowThread>
{
	enum {
		MSG_JOB = WM_USER+1,
	};

	BEGIN_MSG_MAP(CNewzflowThread)
		MESSAGE_HANDLER(MSG_JOB, OnJob)
	END_MSG_MAP()

public:
	CNewzflowThread() : CGuiThreadImpl<CNewzflowThread>(&_Module) {}

	void Add(const CString& nzbUrl);
	LRESULT OnJob(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
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
	void UpdateSegment(CNzbSegment* s, ENzbStatus newStatus);
	void RemoveDownloader(CDownloader* dl);
	CAtlArray<CNzb*> nzbs;
	CAtlArray<CDownloader*> downloaders, finishedDownloaders;
	CDiskWriter* diskWriter;
	CPostProcessor* postProcessor;
	CNewzflowThread* controlThread;

	CComAutoCriticalSection cs;

protected:
	static CNewzflow* s_pInstance;

	bool shuttingDown;
};
