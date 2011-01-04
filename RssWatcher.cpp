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
		{
			sq3::Statement st(CNewzflow::Instance()->database->database, _T("SELECT rowid, description FROM RssItems WHERE description like ?"));
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
				//ProcessMovie(id, imdbId);
				//Util::PostMessageToMainWindow(Util::MSG_TVSHOW_UPDATED);
			}
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
	// first check if movie is already in local database
	QMovies* movies = CNewzflow::Instance()->database->GetMovies(imdbId);
	int movieId = 0;
	if(movies->GetRow())
		movieId = movies->GetId();
	delete movies;
	if(movieId == 0) {
		// if not, get it from TMDB
		TheMovieDB::CGetMovie* getMovie = CNewzflow::Instance()->tmdbApi->GetMovie(imdbId);
		if(getMovie && !getMovie->Movies.IsEmpty()) {
			TheMovieDB::CMovie* movie = getMovie->Movies[0];
			// find and download poster image
			int posterIndex = movie->images.Find(_T("poster"), _T("cover"));
			if(posterIndex != -1) {
				CString posterPath;
				posterPath.Format(_T("%smovie%d.jpg"), CNewzflow::Instance()->settings->GetAppDataDir(), movie->id);
				downloader.Download(movie->images[posterIndex]->url, posterPath, CString(), NULL);
			}

			// find and download actor images
			for(int i = movie->cast.Find(_T("Actors"), NULL); i != -1; i = movie->cast.Find(_T("Actors"), NULL, i)) {
				TheMovieDB::CPerson* person = movie->cast[i];
				TheMovieDB::CGetPerson* getPerson = CNewzflow::Instance()->tmdbApi->GetPerson(person->id);
				if(getPerson && !getPerson->Persons.IsEmpty()) {
					person = getPerson->Persons[0];
					int profileIndex = person->images.Find(_T("profile"), _T("profile"));
					if(profileIndex != -1) {
						CString profilePath;
						profilePath.Format(_T("%sperson%d.jpg"), CNewzflow::Instance()->settings->GetAppDataDir(), person->id);
						downloader.Download(person->images[profileIndex]->url, profilePath, CString(), NULL);
					}
				}
				delete getPerson;
			}

			CTransaction transaction(CNewzflow::Instance()->database);
			movieId = CNewzflow::Instance()->database->InsertMovie(imdbId, movie);
		}
		delete getMovie;
	}
	// insert release
	if(movieId > 0) {
		CNewzflow::Instance()->database->InsertMovieRelease(movieId, id);
	}
}
