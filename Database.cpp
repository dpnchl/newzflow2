#include "stdafx.h"
#include "Database.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// CQuery<>
//////////////////////////////////////////////////////////////////////////

// CDatabase
//////////////////////////////////////////////////////////////////////////

CDatabase::CDatabase(const CString& dbPath)
{
	database.Open(dbPath);
	ASSERT(database.IsOpen());
	{ CTransaction transaction(this);
		database.Execute("CREATE TABLE IF NOT EXISTS \"RssFeeds\" (\"title\" TEXT, \"url\" TEXT NOT NULL UNIQUE, \"update_interval\" INTEGER DEFAULT 15, \"last_update\" REAL)");
		database.Execute("CREATE TABLE IF NOT EXISTS \"RssItems\" (\"feed\" INTEGER, \"title\" TEXT NOT NULL, \"link\" TEXT NOT NULL, \"length\" INTEGER DEFAULT 0, \"description\" TEXT, \"category\" TEXT, \"status\" INTEGER, \"date\" REAL, UNIQUE (feed, title))");
		database.Execute("CREATE TABLE IF NOT EXISTS \"TvShows\" (\"title\" TEXT, \"tvdb_id\" INTEGER, \"last_update\" REAL, \"description\" TEXT)");
		database.Execute("CREATE TABLE IF NOT EXISTS \"TvEpisodes\" (\"show_id\" INTEGER, \"tvdb_id\" INTEGER, \"title\" TEXT, \"season\" INTEGER, \"episode\" INTEGER, \"description\" TEXT, \"date\" REAL)");
		database.Execute("CREATE TABLE IF NOT EXISTS \"Movies\" (\"title\" TEXT, \"imdb_id\" TEXT, \"tmdb_id\" INTEGER NOT NULL UNIQUE)");
		database.Execute("CREATE TABLE IF NOT EXISTS \"MovieActors\" (\"movie\" INTEGER, \"actor\" INTEGER, \"order\" INTEGER)");
		database.Execute("CREATE TABLE IF NOT EXISTS \"Actors\" (\"name\" TEXT, \"tmdb_id\" INTEGER NOT NULL UNIQUE)");
	}
}

CDatabase::~CDatabase()
{
}

QRssItems* CDatabase::GetRssItems(int feedId)
{
	return new QRssItems(this, feedId);
}

QTvEpisodes* CDatabase::GetTvEpisodes(int showId)
{
	return new QTvEpisodes(this, showId);
}

QMovies* CDatabase::GetMovies()
{
	return new QMovies(this);
}

QActors* CDatabase::GetActors(int movieId)
{
	return new QActors(this, movieId);
}

CString CDatabase::DownloadRssItem(int id)
{
	CString sUrl;
	// get URL for RSS item and add NZB
	{
		CQuery q(this);
		q.Prepare(_T("SELECT link FROM RssItems WHERE rowid = ?"), id);
		sUrl = q.ExecuteSingle<CString>();
	}

	// set status to downloaded
	{
		CQuery q(this);
		q.Prepare(_T("UPDATE RssItems SET status = ? WHERE rowid = ?"), QRssItems::kDownloaded, id);
		q.ExecuteVoid();
	}

	return sUrl;
}

void CDatabase::InsertTvEpisode(int showId, TheTvDB::CEpisode* episode)
{
	CQuery q(this);
	q.Prepare(_T("INSERT OR IGNORE INTO TvEpisodes (show_id, tvdb_id, title, season, episode, description, date) VALUES (?, ?, ?, ?, ?, ?, julianday(?))"));
	q.Bind(0, showId);
	q.Bind(1, episode->id);
	q.Bind(2, episode->EpisodeName);
	q.Bind(3, episode->SeasonNumber);
	q.Bind(4, episode->EpisodeNumber);
	q.Bind(5, episode->Overview);
	q.Bind(6, episode->FirstAired.Format(_T("%Y-%m-%d")));
	q.ExecuteInsert();
}

void CDatabase::UpdateTvShow(int showId)
{
	// set last update
	CQuery q(this);
	q.Prepare(_T("UPDATE TvShows SET last_update = julianday('now') WHERE rowid = ?"), showId);
	q.ExecuteVoid();
}

void CDatabase::InsertRssItem(int feedId, CRssItem* item)
{
	CQuery q(this);
	q.Prepare(_T("INSERT OR IGNORE INTO RssItems (feed, title, link, length, description, category, date) VALUES (?, ?, ?, ?, ?, ?, julianday(?))"));
	q.Bind(0, feedId);
	q.Bind(1, item->title);
	if(item->enclosure) {
		q.Bind(2, item->enclosure->url);
		q.Bind(3, item->enclosure->length);
	} else {
		q.Bind(2, item->link);
		q.Bind(3, 0);
	}
	q.Bind(4, item->description);
	q.Bind(5, item->category);
	q.Bind(6, item->pubDate.Format(_T("%Y-%m-%d %H:%M:%S")));
	q.ExecuteInsert();
}

void CDatabase::UpdateRssFeed(int feedId, CRss* rss)
{
	// set update interval
	if(rss->ttl > 0) {
		CQuery q(this);
		q.Prepare(_T("UPDATE RssFeeds SET update_interval = ? WHERE rowid = ?"), rss->ttl, feedId);
		q.ExecuteVoid();
	}
	// set last update
	{
		CQuery q(this);
		q.Prepare(_T("UPDATE RssFeeds SET last_update = julianday('now') WHERE rowid = ?"), feedId);
		q.ExecuteVoid();
	}
	// set title from feed if previously empty
	{
		CQuery q(this);
		q.Prepare(_T("UPDATE RssFeeds SET title = ? WHERE rowid = ? AND title ISNULL"), rss->title, feedId);
		q.ExecuteVoid();
	}
}

void CDatabase::InsertMovie(const CString& imdbId, TheMovieDB::CMovie* movie)
{
	CQuery q(this);
	q.Prepare(_T("INSERT OR IGNORE INTO Movies (imdb_id, tmdb_id, title) VALUES (?, ?, ?)"));
	q.Bind(0, imdbId);
	q.Bind(1, movie->id);
	q.Bind(2, movie->name);
	__int64 movieId = q.ExecuteInsert();

	if(movieId != 0) {
		for(int i = movie->cast.Find(_T("Actors"), NULL); i != -1; i = movie->cast.Find(_T("Actors"), NULL, i)) {
			TheMovieDB::CPerson* actor = movie->cast[i];
			q.Prepare(_T("SELECT rowid FROM Actors WHERE tmdb_id = ?"), actor->id);
			__int64 actorId = q.ExecuteSingle<__int64>();
			if(actorId == 0) {
				q.Prepare(_T("INSERT INTO Actors (tmdb_id, name) VALUES (?, ?)"), actor->id, actor->name);
				actorId = q.ExecuteInsert();
			}
			q.Prepare(_T("INSERT INTO MovieActors (movie, actor, \"order\") VALUES (?, ?, ?)"), movieId, actorId, actor->order);
			q.ExecuteInsert();
		}
	}
}

