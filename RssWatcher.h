#pragma once

#include "HttpDownloader.h"

class CRssWatcher : public CThreadImpl<CRssWatcher>
{
public:
	CRssWatcher();

	DWORD Run();
	void ProcessFeed(int id, const CString& sUrl);
	void ProcessTvShow(int id, int tvdb_id);
	void ProcessMovie(int id, const CString& imdbId);

	CEvent shutDown; // CNewzflow will set this event to indicate the dir watcher should shut down
	CEvent refresh; // CNewzflow will set this event to force a refresh (when a new feed has been added)

protected:
	CHttpDownloader downloader;
};