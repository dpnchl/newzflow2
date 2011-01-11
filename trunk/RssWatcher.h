#pragma once

#include "HttpDownloader.h"

class IProvider;

class CRssWatcher : public CThreadImpl<CRssWatcher>
{
public:
	CRssWatcher();

	DWORD Run();
	void ProcessFeed(int id, const CString& sUrl);
	void ProcessTvShow(int id, int tvdb_id);
	void ProcessMovie(int id, const CString& imdbId);

	void SetLastCommand(const CString& s);
	void SetLastReply(const CString& s);
	CString GetLastCommand();
	CString GetLastReply();

	CEvent shutDown; // CNewzflow will set this event to indicate the dir watcher should shut down
	CEvent refresh; // CNewzflow will set this event to force a refresh (when a new feed has been added)
	CEvent settingsChanged; // CNewzflow will set this event after rss/provider settings have been changed and the providers should be notified

protected:
	CHttpDownloader downloader;
	CAtlArray<IProvider*> providers;

	CComAutoCriticalSection cs; // protects lastCommand and lastReply
	CString lastCommand, lastReply;
};