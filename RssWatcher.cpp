#include "stdafx.h"
#include "util.h"
#include "Rss.h"
#include "RssWatcher.h"
#include "Newzflow.h"
#include "Settings.h"
#include "TheTvDB.h"
#include "TheMovieDB.h"
#include "Database.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// CRssWatcher
//////////////////////////////////////////////////////////////////////////

CRssWatcher::CRssWatcher()
: CThreadImpl<CRssWatcher>(CREATE_SUSPENDED)
, shutDown(TRUE, FALSE)
, refresh(FALSE, FALSE)
{
	Resume();
}

DWORD CRssWatcher::Run()
{
	int delay = 0;
	downloader.Init();

	for(;;) {
		if(CNewzflow::Instance()->IsShuttingDown() || WaitForSingleObject(shutDown, 0) == WAIT_OBJECT_0)
			break;

		if(delay > 0) {
			HANDLE objs[] = { shutDown, refresh };
			WaitForMultipleObjects(2, objs, FALSE, delay * 1000);
			delay = 0;
			continue;
		}

		// Process RSS Feeds
		{
			sq3::Statement st(CNewzflow::Instance()->database->database, _T("SELECT rowid, url, title FROM RssFeeds WHERE last_update ISNULL OR ((strftime('%s', 'now') - strftime('%s', last_update)) / 60) >= update_interval"));
			sq3::Reader reader = st.ExecuteReader();
			while(reader.Step() == SQLITE_ROW) {
				int id; reader.GetInt(0, id);
				CString sUrl; reader.GetString(1, sUrl);
				CString sTitle; reader.GetString(2, sTitle);
				CString s; s.Format(_T("ProcessFeed(%s (%d), %s)\n"), sTitle, id, sUrl); Util::Print(s);
				ProcessFeed(id, sUrl);
				Util::PostMessageToMainWindow(Util::MSG_RSSFEED_UPDATED);
			}
		}
		// Process TV Shows
		{
			sq3::Statement st(CNewzflow::Instance()->database->database, _T("SELECT rowid, tvdb_id, title FROM TvShows WHERE last_update ISNULL")); // OR ((strftime('%s', 'now') - strftime('%s', last_update)) / 60) >= update_interval"));
			sq3::Reader reader = st.ExecuteReader();
			while(reader.Step() == SQLITE_ROW) {
				int id; reader.GetInt(0, id);
				int tvdb_id; reader.GetInt(1, tvdb_id);
				CString sTitle; reader.GetString(2, sTitle);
				CString s; s.Format(_T("ProcessTvShow(%s (%d), tvdb_id=%d)\n"), sTitle, id, tvdb_id); Util::Print(s);
				ProcessTvShow(id, tvdb_id);
				Util::PostMessageToMainWindow(Util::MSG_TVSHOW_UPDATED);
			}
		}
		// Process Movies; TEST
/*		{
			sq3::Statement st(CNewzflow::Instance()->database->database, _T("SELECT rowid, description FROM RssItems WHERE description like ? LIMIT 1"));
			st.Bind(0, _T("%www.imdb.com/title/tt%"));
			sq3::Reader reader = st.ExecuteReader();
			while(reader.Step() == SQLITE_ROW) {
				int id; reader.GetInt(0, id);
				CString sDescription; reader.GetString(1, sDescription);
				CString imdbId;
				int imdbStart = sDescription.Find(_T("www.imdb.com/title/tt"));
				if(imdbStart > 0)
					imdbId = sDescription.Mid(imdbStart + _tcslen(_T("www.imdb.com/title/")), 9);
				CString s; s.Format(_T("ProcessMovie(%s (%d))\n"), imdbId, id); Util::Print(s);
				ProcessMovie(id, imdbId);
				//Util::PostMessageToMainWindow(Util::MSG_TVSHOW_UPDATED);
			}
*/
		}

		delay = 60;
	}

	return 0;
}

void CRssWatcher::ProcessFeed(int id, const CString& sUrl)
{
	CString rssFile = CNewzflow::Instance()->settings->GetAppDataDir() + _T("temp.rss");
	if(downloader.Download(sUrl, rssFile, CString(), NULL)) {
		CRss rss;
		if(rss.Parse(rssFile)) {
			CTransaction transaction(CNewzflow::Instance()->database);
			for(size_t i = 0; i < rss.items.GetCount(); i++) {
				CRssItem* item = rss.items[i];
				CNewzflow::Instance()->database->InsertRssItem(id, item);
			}
			CNewzflow::Instance()->database->UpdateRssFeed(id, &rss);
		}
	}
	CFile::Delete(rssFile);
}

void CRssWatcher::ProcessTvShow(int id, int tvdb_id)
{
	TheTvDB::CSeriesAll* seriesAll = CNewzflow::Instance()->tvdbApi->SeriesAll(tvdb_id);
	if(seriesAll) {
		CTransaction transaction(CNewzflow::Instance()->database);
		for(size_t i = 0; i < seriesAll->Episodes.GetCount(); i++) {
			TheTvDB::CEpisode* episode = seriesAll->Episodes[i];
			CNewzflow::Instance()->database->InsertTvEpisode(id, episode);
		}
		CNewzflow::Instance()->database->UpdateTvShow(id);
		TRACE(_T("inserted %d episodes\n"), seriesAll->Episodes.GetCount());
	}
	delete seriesAll;
}

void CRssWatcher::ProcessMovie(int id, const CString& imdbId)
{
	TheMovieDB::CGetMovie* getMovie = CNewzflow::Instance()->tmdbApi->GetMovie(imdbId);
/*	if(getMovie) {
		CTransaction transaction(CNewzflow::Instance()->database);
		for(size_t i = 0; i < getMovie->Episodes.GetCount(); i++) {
			TheTvDB::CEpisode* episode = getMovie->Episodes[i];
			CNewzflow::Instance()->database->InsertTvEpisode(id, episode);
		}
		CNewzflow::Instance()->database->UpdateTvShow(id);
		TRACE(_T("inserted %d episodes\n"), getMovie->Episodes.GetCount());
	}
*/
	delete getMovie;
}
