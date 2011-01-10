#include "stdafx.h"
#include "util.h"
#include "Rss.h"
#include "RssWatcher.h"
#include "Newzflow.h"
#include "Settings.h"
#include "TheTvDB.h"
#include "TheMovieDB.h"
#include "Database.h"
#include "Provider.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// CRssWatcher
//////////////////////////////////////////////////////////////////////////

CRssWatcher::CRssWatcher()
: CThreadImpl<CRssWatcher>(CREATE_SUSPENDED)
, shutDown(TRUE, FALSE)
, refresh(FALSE, FALSE)
, settingsChanged(FALSE, FALSE)
{
	Resume();
}

DWORD CRssWatcher::Run()
{
	int delay = 0;
	downloader.Init();

	providers.Add(new CProviderNzbMatrix);

	for(;;) {
		if(CNewzflow::Instance()->IsShuttingDown() || WaitForSingleObject(shutDown, 0) == WAIT_OBJECT_0)
			break;

		if(delay > 0) {
			HANDLE objs[] = { shutDown, refresh, settingsChanged };
			switch(WaitForMultipleObjects(3, objs, FALSE, delay * 1000)) {
			case WAIT_OBJECT_0 + 2: // settingsChanged
				for(size_t i = 0; i < providers.GetCount(); i++)
					providers[i]->OnSettingsChanged();
				break;
			}
			delay = 0;
			continue;
		}

		// Process RSS Feeds
		QRssFeedsToRefresh* rss = CNewzflow::Instance()->database->GetRssFeedsToRefresh();
		while(rss->GetRow()) {
			CString s; s.Format(_T("ProcessFeed(%s (%d), %s)\n"), rss->GetTitle(), rss->GetId(), rss->GetUrl()); Util::Print(s);
			ProcessFeed(rss->GetId(), rss->GetUrl());
			Util::PostMessageToMainWindow(Util::MSG_RSSFEED_UPDATED);
		}
		delete rss;

		// Process Providers
		for(size_t i = 0; i < providers.GetCount(); i++)
			providers[i]->Process();

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

		delay = 60;
	}

	for(size_t i = 0; i < providers.GetCount(); i++)
		delete providers[i];

	return 0;
}

void CRssWatcher::ProcessFeed(int id, const CString& sUrl)
{
	CString rssFile = CNewzflow::Instance()->settings->GetAppDataDir() + _T("temp.rss");
	if(downloader.Download(sUrl, rssFile, CString(), NULL)) {
		CRss rss;
		if(rss.Parse(rssFile, sUrl)) {
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
