#pragma once

#include "SQLite/SQ3.h"
#include "TheTvDB.h"
#include "TheMovieDB.h"
#include "rss.h"

class CDatabase
{
public:
	CDatabase(const CString& dbPath);
	~CDatabase();

	class QRssItems* GetRssItemsByFeed(int feedId);
	class QRssItems* GetRssItemsByMovie(int movieId);
	CString DownloadRssItem(int id);
	int InsertRssItem(int feedId, CRssItem* item, int quality = -1);

	class QRssFeeds* GetRssFeeds(int id = 0);
	class QRssFeedsToRefresh* GetRssFeedsToRefresh(const CString& provider = _T(""));
	void InsertRssFeed(int id, const CString& title, const CString& url, bool clearLastUpdate = true);
	void UpdateRssFeed(int id, CRss* rss);
	void DeleteRssFeed(int id);

	class QTvEpisodes* GetTvEpisodes(int showId);
	void InsertTvEpisode(int showId, TheTvDB::CEpisode* episode);

	class QTvShows* GetTvShows(int id = 0);
	void InsertTvShow(TheTvDB::CSeries* series);
	void UpdateTvShow(int showId);

	class QMovies* GetMovies(const CString& imdbId = _T(""));
	int InsertMovie(const CString& imdbId, TheMovieDB::CMovie* movie);
	void CleanupMovies();

	class QActors* GetActors(int movieId);
	void InsertMovieRelease(int movieId, int rssItem);
	void DeleteTvShow(int showId);
//protected:
	sq3::Database database;
	CComAutoCriticalSection insertCs, transactionCs;
	sq3::Transaction* volatile transaction;
};

// auto-commit transaction
class CTransaction
{
public:
	CTransaction(CDatabase* _database)
	{
		database = _database;
		database->transactionCs.Lock();
		if(!database->transaction) {
			database->transaction = new sq3::Transaction(database->database);
			own = true;
		} else {
			own = false;
		}
		rollback = false;
	}
	~CTransaction()
	{
		if(own) {
			if(!rollback)
				database->transaction->Commit();
			delete database->transaction;
			database->transaction = NULL;
		}
		database->transactionCs.Unlock();
	}
	void Rollback()
	{
		if(own) {
			database->transaction->Rollback();
		}
		rollback = true;
	}

protected:
	CDatabase* database;
	bool own;
	bool rollback;
};

class CQuery
{
public:
	CQuery(CDatabase* _database)
	{
		database = _database;
		statement = NULL;
		reader = NULL;
	}
	~CQuery()
	{
		delete reader;
		delete statement;
	}
	void Prepare(const CString& query)
	{
		delete reader; reader = NULL;
		delete statement; statement = NULL;
		statement = new sq3::Statement(database->database, query);
		ASSERT(statement->IsValid());
	}
	template <class T1>
	void Prepare(const CString& query, T1 val1)
	{
		Prepare(query);
		Bind(0, val1);
	}
	template <class T1, class T2>
	void Prepare(const CString& query, T1 val1, T2 val2)
	{
		Prepare(query);
		Bind(0, val1);
		Bind(1, val2);
	}
	template <class T1, class T2, class T3>
	void Prepare(const CString& query, T1 val1, T2 val2, T3 val3)
	{
		Prepare(query);
		Bind(0, val1);
		Bind(1, val2);
		Bind(2, val3);
	}
	template <class T1, class T2, class T3, class T4>
	void Prepare(const CString& query, T1 val1, T2 val2, T3 val3, T4 val4)
	{
		Prepare(query);
		Bind(0, val1);
		Bind(1, val2);
		Bind(2, val3);
		Bind(3, val4);
	}
	template <class T> void Bind(int num, T val)
	{
		statement->Bind(num, val);
	}
	void Execute()
	{
		reader = new sq3::Reader(statement->ExecuteReader());
	}
	template <class T> T ExecuteSingle()
	{
		Execute();
		GetRow();
		return Get<T>(0);
	}
	bool ExecuteVoid()
	{
		return statement->ExecuteNonQuery() == SQLITE_OK;
	}
	__int64 ExecuteInsert()
	{
		__int64 rowid = 0;
		database->insertCs.Lock();
		if(ExecuteVoid())
			rowid = database->database.GetLastInsertID();
		database->insertCs.Unlock();
		return rowid;
	}
	template <class T> T Get(int id)
	{
		ASSERT(FALSE);
		return NULL;
	}
	template <> int Get(int id)
	{
		int val;
		reader->GetInt(id, val);
		return val;
	}
	template <> __int64 Get(int id)
	{
		__int64 val;
		reader->GetInt64(id, val);
		return val;
	}
	template <> CString Get(int id)
	{
		CString val;
		reader->GetString(id, val);
		return val;
	}
	template <> COleDateTime Get(int id)
	{
		int val;
		reader->GetInt(id, val);
		return COleDateTime((time_t)val);
	}

	bool GetRow()
	{
		return reader->Step() == SQLITE_ROW;
	}

protected:
	CDatabase* database;
	sq3::Statement* statement;
	sq3::Reader* reader;
};

class QRssItems : public CQuery
{
public:
	QRssItems(CDatabase* database)
	: CQuery(database) { }

	int GetId() { return Get<int>(0); }
	CString GetTitle() { return Get<CString>(1); }
	__int64 GetSize() { return Get<__int64>(2); }
	COleDateTime GetDate() { return Get<COleDateTime>(3); }
	int GetStatus() { return Get<int>(4); }
	int GetQuality() { return Get<int>(5); }
	CString GetFeedTitle() { return Get<CString>(6); }

	enum EStatus {
		kDownloaded = 1,
	};
};

class QRssItemsByFeed : public QRssItems
{
public:
	QRssItemsByFeed(CDatabase* database, int feedId)
	: QRssItems(database)
	{
		CString sQuery(_T("SELECT RssItems.rowid, RssItems.title, RssItems.length, strftime('%s', RssItems.date), RssItems.status, RssItems.quality, RssFeeds.title FROM RssItems LEFT JOIN RssFeeds ON RssItems.feed = RssFeeds.rowid"));
		if(feedId > 0)
			sQuery += _T(" WHERE RssFeeds.rowid = ?");
		Prepare(sQuery);
		if(feedId > 0)
			Bind(0, feedId);
		Execute();
	}
};

class QRssItemsByMovie : public QRssItems
{
public:
	QRssItemsByMovie(CDatabase* database, int movieId)
	: QRssItems(database)
	{
		CString sQuery(_T("SELECT RssItems.rowid, RssItems.title, RssItems.length, strftime('%s', RssItems.date), RssItems.status, RssItems.quality, RssFeeds.title FROM MovieReleases LEFT JOIN RssItems ON MovieReleases.rss_item = RssItems.rowid LEFT JOIN RssFeeds ON RssItems.feed = RssFeeds.rowid"));
		if(movieId > 0)
			sQuery += _T(" WHERE MovieReleases.movie = ?");
		Prepare(sQuery);
		if(movieId > 0)
			Bind(0, movieId);
		Execute();
	}
};

class QRssFeeds : public CQuery
{
public:
	QRssFeeds(CDatabase* database, int feedId)
	: CQuery(database)
	{
		CString sQuery(_T("SELECT rowid, title, url FROM RssFeeds"));
		if(feedId > 0)
			sQuery += _T(" WHERE rowid = ?");
		else
			sQuery += _T(" ORDER BY title ASC");
		Prepare(sQuery, feedId);
		if(feedId > 0)
			Bind(0, feedId);
		Execute();
	}
	int GetId() { return Get<int>(0); }
	CString GetTitle() { return Get<CString>(1); }
	CString GetUrl() { return Get<CString>(2); }
};

class QRssFeedsToRefresh : public CQuery
{
public:
	QRssFeedsToRefresh(CDatabase* database, const CString& provider)
	: CQuery(database)
	{
		CString query;
		query.Format(_T("SELECT rowid, title, url, provider, special FROM RssFeeds WHERE provider %s AND (last_update ISNULL OR ((strftime('%%s', 'now') - strftime('%%s', last_update)) / 60) >= update_interval)"), provider.IsEmpty() ? _T("ISNULL") : _T("= ?"));
		Prepare(query);
		if(!provider.IsEmpty())
			Bind(0, provider);
		Execute();
	}
	int GetId() { return Get<int>(0); }
	CString GetTitle() { return Get<CString>(1); }
	CString GetUrl() { return Get<CString>(2); }
	CString GetProvider() { return Get<CString>(3); }
	CString GetSpecial() { return Get<CString>(4); }
};

class QTvShows : public CQuery
{
public:
	QTvShows(CDatabase* database, int showId)
	: CQuery(database)
	{
		CString sQuery(_T("SELECT rowid, title FROM TvShows"));
		if(showId > 0)
			sQuery += _T(" WHERE rowid = ?");
		else
			sQuery += _T(" ORDER BY title ASC");
		Prepare(sQuery);
		if(showId > 0)
			Bind(0, showId);
		Execute();
	}
	int GetId() { return Get<int>(0); }
	CString GetTitle() { return Get<CString>(1); }
};

class QTvEpisodes : public CQuery
{
public:
	QTvEpisodes(CDatabase* database, int showId)
	: CQuery(database)
	{
		CString sQuery(_T("SELECT rowid, season, episode, title, strftime('%s', date) FROM TvEpisodes WHERE show_id = ? ORDER BY episode DESC"));
		Prepare(sQuery, showId);
		Execute();
	}
	int GetId() { return Get<int>(0); }
	int GetSeason() { return Get<int>(1); }
	int GetEpisode() { return Get<int>(2); }
	CString GetTitle() { return Get<CString>(3); }
	COleDateTime GetDate() { return Get<COleDateTime>(4); }
};

class QMovies : public CQuery
{
public:
	QMovies(CDatabase* database, const CString& imdbId)
	: CQuery(database)
	{
		CString sQuery(_T("SELECT Movies.rowid, Movies.title, Movies.imdb_id, Movies.tmdb_id FROM Movies"));
		if(!imdbId.IsEmpty())
			sQuery += _T(" WHERE imdb_id = ?");
		else
			sQuery += _T(" LEFT JOIN MovieReleases ON Movies.rowid = MovieReleases.movie LEFT JOIN RssItems ON MovieReleases.rss_item = RssItems.rowid GROUP BY Movies.rowid ORDER BY MAX(RssItems.date) DESC");
		Prepare(sQuery);
		if(!imdbId.IsEmpty())
			Bind(0, imdbId);
		Execute();
	}
	int GetId() { return Get<int>(0); }
	CString GetTitle() { return Get<CString>(1); }
	CString GetImdbId() { return Get<CString>(2); }
	int GetTmdbId() { return Get<int>(3); }
};

class QActors : public CQuery
{
public:
	QActors(CDatabase* database, int movieId)
	: CQuery(database)
	{
		CString sQuery(_T("SELECT Actors.rowid, Actors.name, Actors.tmdb_id FROM MovieActors INNER JOIN Actors ON MovieActors.actor = Actors.rowid WHERE MovieActors.movie = ? ORDER BY MovieActors.\"order\""));
		Prepare(sQuery, movieId);
		Execute();
	}
	int GetId() { return Get<int>(0); }
	CString GetName() { return Get<CString>(1); }
	int GetTmdbId() { return Get<int>(2); }
};
