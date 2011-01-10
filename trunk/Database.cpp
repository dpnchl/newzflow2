#include "stdafx.h"
#include "Database.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// CDatabase
//////////////////////////////////////////////////////////////////////////

CDatabase::CDatabase(const CString& dbPath)
: transaction(NULL)
{
	database.Open(dbPath);
	ASSERT(database.IsOpen());
	{ CTransaction transaction(this);
		database.Execute("CREATE TABLE IF NOT EXISTS \"RssFeeds\" (\"title\" TEXT, \"url\" TEXT, \"update_interval\" INTEGER DEFAULT 15, \"last_update\" REAL, \"provider\" TEXT, \"special\" TEXT)");
		database.Execute("CREATE TABLE IF NOT EXISTS \"RssItems\" (\"feed\" INTEGER, \"title\" TEXT NOT NULL, \"link\" TEXT NOT NULL, \"length\" INTEGER DEFAULT 0, \"description\" TEXT, \"category\" TEXT, \"status\" INTEGER, \"date\" REAL, \"quality\" INTEGER, UNIQUE (feed, title))");
		database.Execute("CREATE TABLE IF NOT EXISTS \"TvShows\" (\"title\" TEXT, \"tvdb_id\" INTEGER, \"last_update\" REAL, \"description\" TEXT)");
		database.Execute("CREATE TABLE IF NOT EXISTS \"TvEpisodes\" (\"show_id\" INTEGER, \"tvdb_id\" INTEGER, \"title\" TEXT, \"season\" INTEGER, \"episode\" INTEGER, \"description\" TEXT, \"date\" REAL)");
		database.Execute("CREATE TABLE IF NOT EXISTS \"Movies\" (\"title\" TEXT, \"imdb_id\" TEXT, \"tmdb_id\" INTEGER NOT NULL UNIQUE)");
		database.Execute("CREATE TABLE IF NOT EXISTS \"MovieActors\" (\"movie\" INTEGER, \"actor\" INTEGER, \"order\" INTEGER)");
		database.Execute("CREATE TABLE IF NOT EXISTS \"Actors\" (\"name\" TEXT, \"tmdb_id\" INTEGER NOT NULL UNIQUE)");
		database.Execute("CREATE TABLE IF NOT EXISTS \"MovieReleases\" (\"movie\" INTEGER, \"rss_item\" INTEGER NOT NULL UNIQUE)");
	}
}

CDatabase::~CDatabase()
{
	ASSERT(!transaction);
}

QRssItems* CDatabase::GetRssItemsByFeed(int feedId)
{
	return new QRssItemsByFeed(this, feedId);
}

class QRssItems* CDatabase::GetRssItemsByMovie(int movieId)
{
	return new QRssItemsByMovie(this, movieId);
}

class QRssFeeds* CDatabase::GetRssFeeds(int id)
{
	return new QRssFeeds(this, id);
}

class QRssFeedsToRefresh* CDatabase::GetRssFeedsToRefresh(const CString& provider)
{
	return new QRssFeedsToRefresh(this, provider);
}

class QTvShows* CDatabase::GetTvShows(int id /*= 0*/)
{
	return new QTvShows(this, id);
}

QTvEpisodes* CDatabase::GetTvEpisodes(int showId)
{
	return new QTvEpisodes(this, showId);
}

QMovies* CDatabase::GetMovies(const CString& imdbId)
{
	return new QMovies(this, imdbId);
}

QActors* CDatabase::GetActors(int movieId)
{
	return new QActors(this, movieId);
}

CString CDatabase::DownloadRssItem(int id)
{
	CQuery q(this);

	// get URL for RSS item
	q.Prepare(_T("SELECT link FROM RssItems WHERE rowid = ?"), id);
	CString sUrl = q.ExecuteSingle<CString>();

	// set status to downloaded
	q.Prepare(_T("UPDATE RssItems SET status = ? WHERE rowid = ?"), QRssItems::kDownloaded, id);
	q.ExecuteVoid();

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

void CDatabase::InsertTvShow(TheTvDB::CSeries* series)
{
	CQuery q(this);
	q.Prepare(_T("INSERT OR IGNORE INTO TvShows (title, tvdb_id, description) VALUES (?, ?, ?)"));
	q.Bind(0, series->SeriesName);
	q.Bind(1, series->id);
	q.Bind(2, series->Overview);
	q.ExecuteInsert();
}

void CDatabase::UpdateTvShow(int showId)
{
	// set last update
	CQuery q(this);
	q.Prepare(_T("UPDATE TvShows SET last_update = julianday('now') WHERE rowid = ?"), showId);
	q.ExecuteVoid();
}

void CDatabase::DeleteTvShow(int showId)
{
	CTransaction transaction(this);

	CQuery q(this);
	q.Prepare(_T("DELETE FROM TvEpisodes WHERE show_id = ?"), showId);
	q.ExecuteVoid();

	q.Prepare(_T("DELETE FROM TvShows WHERE rowid = ?"), showId);
	q.ExecuteVoid();
}

int CDatabase::InsertRssItem(int feedId, CRssItem* item, int quality /*= -1*/)
{
	CQuery q(this);
	q.Prepare(_T("INSERT INTO RssItems (feed, title, link, length, description, category, date, quality) VALUES (?, ?, ?, ?, ?, ?, julianday(?), ?)"));
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
	if(quality != -1)
		q.Bind(7, quality);
	return (int)q.ExecuteInsert();
}

// INSERT if id=0; UPDATE otherwise
void CDatabase::InsertRssFeed(int id, const CString& title, const CString& url, bool clearLastUpdate /*= true*/)
{
	CTransaction transaction(this);

	CQuery q(this);
	if(id == 0) {
		q.Prepare(_T("INSERT OR IGNORE INTO RssFeeds (title, url) VALUES (?, ?)"), title, url);
		q.ExecuteInsert();
	} else {
		q.Prepare(_T("UPDATE RssFeeds SET title = ?, url = ? WHERE rowid = ?"), title, url, id);
		q.ExecuteVoid();
		if(clearLastUpdate) {
			q.Prepare(_T("UPDATE RssFeeds SET last_update = NULL WHERE rowid = ?"), id);
			q.ExecuteVoid();
		}
	}
}

void CDatabase::UpdateRssFeed(int feedId, CRss* rss)
{
	CTransaction transaction(this);

	CQuery q(this);
	// set update interval
	if(rss && rss->ttl > 0) {
		q.Prepare(_T("UPDATE RssFeeds SET update_interval = ? WHERE rowid = ?"), rss->ttl, feedId);
		q.ExecuteVoid();
	}

	// set last update
	q.Prepare(_T("UPDATE RssFeeds SET last_update = julianday('now') WHERE rowid = ?"), feedId);
	q.ExecuteVoid();

	// set title from feed if previously empty
	if(rss) {
		q.Prepare(_T("UPDATE RssFeeds SET title = ? WHERE rowid = ? AND title = ''"), rss->title, feedId);
		q.ExecuteVoid();
	}
}

void CDatabase::DeleteRssFeed(int id)
{
	CTransaction transaction(this);

	CQuery q(this);
	q.Prepare(_T("DELETE FROM MovieReleases WHERE rss_item IN (SELECT rowid FROM RssItems WHERE feed = ?)"), id);
	q.ExecuteVoid();

	q.Prepare(_T("DELETE FROM RssItems WHERE feed = ?"), id);
	q.ExecuteVoid();

	q.Prepare(_T("DELETE FROM RssFeeds WHERE rowid = ?"), id);
	q.ExecuteVoid();

	CleanupMovies();
}

int CDatabase::InsertMovie(const CString& imdbId, TheMovieDB::CMovie* movie)
{
	CTransaction transaction(this);

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
	return (int)movieId;
}

void CDatabase::CleanupMovies()
{
	CTransaction transaction(this);

	CQuery q(this);

	// delete movies that have no releases
	q.Prepare(_T("DELETE FROM Movies WHERE rowid IN (SELECT Movies.rowid FROM Movies LEFT JOIN MovieReleases ON Movies.rowid = MovieReleases.movie WHERE MovieReleases.movie ISNULL)"));
	q.ExecuteVoid();

	// delete cast that has no movie
	q.Prepare(_T("DELETE FROM MovieActors WHERE rowid IN (SELECT MovieActors.rowid FROM MovieActors LEFT JOIN Movies ON MovieActors.movie = Movies.rowid WHERE Movies.rowid ISNULL)"));
	q.ExecuteVoid();

	// delete actors that have no cast
	q.Prepare(_T("DELETE FROM Actors WHERE rowid IN (SELECT Actors.rowid FROM Actors LEFT JOIN MovieActors ON Actors.rowid = MovieActors.actor WHERE MovieActors.actor ISNULL)"));
	q.ExecuteVoid();
}

void CDatabase::InsertMovieRelease(int movieId, int rssItem)
{
	CQuery q(this);
	q.Prepare(_T("INSERT OR IGNORE INTO MovieReleases (movie, rss_item) VALUES (?, ?)"), movieId, rssItem);
	q.ExecuteInsert();
}
