#pragma once

#include "SQLite/SQ3.h"
#include "TheTvDB.h"
#include "rss.h"

class CDatabase
{
public:
	CDatabase(const CString& dbPath);
	~CDatabase();

	class QRssItems* GetRssItems(int feedId);
	CString DownloadRssItem(int id);
	void InsertRssItem(int feedId, CRssItem* item);

	void UpdateTvShow(int showId);

	class QTvEpisodes* GetTvEpisodes(int showId);
	void InsertTvEpisode(int showId, TheTvDB::CEpisode* episode);
	void UpdateRssFeed(int id, CRss* rss);
	sq3::Database database;
};

class CTransaction
{
public:
	CTransaction(CDatabase* database)
		: transaction(database->database)
	{
		rollback = false;
	}
	~CTransaction()
	{
		if(!rollback)
			transaction.Commit();
	}
	void Rollback()
	{
		transaction.Rollback();
		rollback = true;
	}

protected:
	sq3::Transaction transaction;
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
	QRssItems(CDatabase* database, int feedId)
	: CQuery(database)
	{
		CString sQuery(_T("SELECT RssItems.rowid, RssItems.title, RssItems.length, strftime('%s', RssItems.date), RssItems.status, RssFeeds.title FROM RssItems LEFT JOIN RssFeeds ON RssItems.feed = RssFeeds.rowid"));
		if(feedId > 0)
			sQuery += _T(" WHERE RssFeeds.rowid = ?");
		Prepare(sQuery);
		if(feedId > 0)
			Bind(0, feedId);
		Execute();
	}
	int GetId() { return Get<int>(0); }
	CString GetTitle() { return Get<CString>(1); }
	__int64 GetSize() { return Get<__int64>(2); }
	COleDateTime GetDate() { return Get<COleDateTime>(3); }
	int GetStatus() { return Get<int>(4); }
	CString GetFeedTitle() { return Get<CString>(5); }

	enum EStatus {
		kDownloaded = 1,
	};
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
