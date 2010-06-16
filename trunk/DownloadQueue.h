#pragma once

class CNzbFile;
class CNzbSegment;

class CDownloadQueue
{
public:
	class CEntry {
	public:
		CNzbFile* file;
		CNzbSegment* segment;
	};

	CDownloadQueue()
	{
		ASSERT(s_pInstance == NULL);
		s_pInstance = this;
	}
	~CDownloadQueue()
	{
		s_pInstance = NULL;
	}
	static CDownloadQueue* Instance()
	{
		return s_pInstance;
	}

	void Add(CNzbFile* nzbfile);
	CEntry* Get();

protected:
	CAtlList<CEntry*> list;
	CComAutoCriticalSection cs;

	static CDownloadQueue* s_pInstance;
};