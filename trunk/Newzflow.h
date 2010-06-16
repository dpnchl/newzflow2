#pragma once

#include "nzb.h"

class CDownloader;

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
	CAtlArray<CNzb*> nzbs;
	CAtlArray<CDownloader*> downloaders;

	CComAutoCriticalSection cs;

protected:
	static CNewzflow* s_pInstance;
};
