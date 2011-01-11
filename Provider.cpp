#include "stdafx.h"
#include "Newzflow.h"
#include "Database.h"
#include "Provider.h"
#include "Settings.h"
#include "RssWatcher.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// IProvider
//////////////////////////////////////////////////////////////////////////

IProvider::IProvider()
{
}

IProvider::~IProvider()
{
}

// CProviderNzbMatrix
//////////////////////////////////////////////////////////////////////////

CProviderNzbMatrix::CProviderNzbMatrix()
{
	downloader.Init();
}

CProviderNzbMatrix::~CProviderNzbMatrix()
{
}

bool CProviderNzbMatrix::Process()
{
	CRssWatcher* rssWatcher = CNewzflow::Instance()->rssWatcher;
	// Process RSS Feeds
	QRssFeedsToRefresh* rss = CNewzflow::Instance()->database->GetRssFeedsToRefresh(_T("nzbmatrix.com"));
	while(rss->GetRow()) {
		CString s; s.Format(_T("Refreshing NZB Provider: %s"), rss->GetProvider());
		rssWatcher->SetLastCommand(s); rssWatcher->SetLastReply(_T(""));
		if(rss->GetSpecial() == _T("Movies"))
			ProcessMovies(rss->GetId());
	}
	delete rss;

	return true;
}

void CProviderNzbMatrix::OnSettingsChanged()
{
	CSettings* settings = CNewzflow::Instance()->settings;
	if(settings->GetIni(_T("nzbmatrix.com"), _T("Use"), _T("0")) != _T("0")) {
		// create new virtual rss feed for movies
		CQuery q(CNewzflow::Instance()->database);
		q.Prepare(_T("SELECT rowid FROM RssFeeds WHERE provider = ? AND special = ?"), _T("nzbmatrix.com"), _T("Movies"));
		__int64 feedId = q.ExecuteSingle<__int64>();
		if(feedId) {
			// reset last_update, so we get updated immediately
			q.Prepare(_T("UPDATE RssFeeds SET last_update = NULL WHERE rowid = ?"), feedId);
			q.ExecuteVoid();
		} else {
			// insert new feed
			q.Prepare(_T("INSERT INTO RssFeeds (title, update_interval, provider, special) VALUES (?, ?, ?, ?)"), _T("nzbmatrix.com Movies"), 25, _T("nzbmatrix.com"), _T("Movies"));
			feedId = q.ExecuteInsert();
		}

		// delete releases with qualities we no longer want
		{ CTransaction transaction(CNewzflow::Instance()->database);
			unsigned qualityMask = _ttoi(settings->GetIni(_T("Movies"), _T("QualityMask"), _T("0")));
			for(int i = 0; i < CSettings::mqMax; i++) {
				if(qualityMask & (1 << i))
					continue;
				q.Prepare(_T("DELETE FROM MovieReleases WHERE rss_item IN (SELECT rowid FROM RssItems WHERE quality = ?)"), i);
				q.ExecuteVoid();
				q.Prepare(_T("DELETE FROM RssItems WHERE quality = ?"), i);
				q.ExecuteVoid();
			}
			CNewzflow::Instance()->database->CleanupMovies();
		}
		Util::PostMessageToMainWindow(Util::MSG_RSSFEED_UPDATED);
	} else {
		// delete all feeds for this provider
		CTransaction transaction(CNewzflow::Instance()->database);
		CQuery q(CNewzflow::Instance()->database);
		q.Prepare(_T("SELECT rowid FROM RssFeeds WHERE provider = ?"), _T("nzbmatrix.com"));
		while(q.GetRow()) {
			CNewzflow::Instance()->database->DeleteRssFeed(q.Get<int>(0));
		}
	}
}

CRss* CProviderNzbMatrix::GetRssFeed(int category, const CString& term)
{
	CSettings* settings = CNewzflow::Instance()->settings;
	CString rssFile = CNewzflow::Instance()->settings->GetAppDataDir() + _T("temp.rss");

	CString sUrl;
	sUrl.Format(_T("http://rss.nzbmatrix.com/rss.php?page=download&username=%s&apikey=%s&subcat=%d&scenename=1&term=%s"), 
		settings->GetIni(_T("nzbmatrix.com"), _T("User")), 
		settings->GetIni(_T("nzbmatrix.com"), _T("ApiKey")), 
		category, 
		term
	);

	if(downloader.Download(sUrl, rssFile, CString(), NULL)) {
		CRss* rss = new CRss;
		if(rss->Parse(rssFile, sUrl)) {
			CFile::Delete(rssFile);
			return rss;
		} else {
			CString sError;
			sError.Format(_T("%s:Error parsing RSS feed %s\n"), __FUNCTIONW__, sUrl);
			CFile::Delete(rssFile);
			delete rss;
			return NULL;
		}
	} else {
		CString sError;
		sError.Format(_T("%s:Error downloading RSS feed %s\n"), __FUNCTIONW__, sUrl);
		return NULL;
	}
}

void CProviderNzbMatrix::ProcessMovies(int feedId)
{
	CRssWatcher* rssWatcher = CNewzflow::Instance()->rssWatcher;
	CSettings* settings = CNewzflow::Instance()->settings;
	unsigned qualityMask = _ttoi(settings->GetIni(_T("Movies"), _T("QualityMask"), _T("0")));

	// TODO: make wait interruptible (for app shutdown)
	time_t startTime = 0;

	if(qualityMask & (1 << CSettings::mqBD)) {
		while(time(NULL) - startTime < 10) Sleep(10*1000);
		startTime = time(NULL);

		CRss* rss = GetRssFeed(50, CString());
		if(rss) {
			CTransaction transaction(CNewzflow::Instance()->database);
			for(size_t i = 0; i < rss->items.GetCount(); i++) {
				CRssItem* item = rss->items[i];
				int quality = CSettings::mqBD;
				InsertMovie(feedId, item, quality);
			}
			delete rss;
		}
	}

	unsigned qmMKV = qualityMask & ((1 << CSettings::mqMKV1080p) | (1 << CSettings::mqMKV720p));
	if(qmMKV) {
		while(time(NULL) - startTime < 10) Sleep(10*1000);
		startTime = time(NULL);

		CString term;
		if(qmMKV == ((1 << CSettings::mqMKV1080p) | (1 << CSettings::mqMKV720p)))
			term = _T("(1080p,720p)");
		else if(qmMKV == (1 << CSettings::mqMKV1080p))
			term = _T("1080p");
		else if(qmMKV == (1 << CSettings::mqMKV720p))
			term = _T("720p");

		CRss* rss = GetRssFeed(42, term);
		if(rss) {
			CTransaction transaction(CNewzflow::Instance()->database);
			for(size_t i = 0; i < rss->items.GetCount(); i++) {
				CRssItem* item = rss->items[i];
				int quality = -1;
				if(item->title.Find(_T("1080p")) >= 0)
					quality = CSettings::mqMKV1080p;
				else if(item->title.Find(_T("720p")) >= 0)
					quality = CSettings::mqMKV720p;
				InsertMovie(feedId, item, quality);
			}
			delete rss;
		}
	}

	unsigned qmWMV = qualityMask & ((1 << CSettings::mqWMV1080p) | (1 << CSettings::mqWMV720p));
	if(qmWMV) {
		while(time(NULL) - startTime < 10) Sleep(10*1000);
		startTime = time(NULL);

		CString term;
		if(qmWMV == ((1 << CSettings::mqWMV1080p) | (1 << CSettings::mqWMV720p)))
			term = _T("(1080p,720p)");
		else if(qmWMV == (1 << CSettings::mqWMV1080p))
			term = _T("1080p");
		else if(qmWMV == (1 << CSettings::mqWMV720p))
			term = _T("720p");

		CRss* rss = GetRssFeed(48, term);
		if(rss) {
			CTransaction transaction(CNewzflow::Instance()->database);
			for(size_t i = 0; i < rss->items.GetCount(); i++) {
				CRssItem* item = rss->items[i];
				int quality = -1;
				if(item->title.Find(_T("1080p")) >= 0)
					quality = CSettings::mqWMV1080p;
				else if(item->title.Find(_T("720p")) >= 0)
					quality = CSettings::mqWMV720p;
				InsertMovie(feedId, item, quality);
			}
			delete rss;
		}
	}

	if(qualityMask & (1 << CSettings::mqDVD)) {
		while(time(NULL) - startTime < 10) Sleep(10*1000);
		startTime = time(NULL);

		CRss* rss = GetRssFeed(1, CString());
		if(rss) {
			CTransaction transaction(CNewzflow::Instance()->database);
			for(size_t i = 0; i < rss->items.GetCount(); i++) {
				CRssItem* item = rss->items[i];
				int quality = CSettings::mqDVD;
				InsertMovie(feedId, item, quality);
			}
			delete rss;
		}
	}

	if(qualityMask & (1 << CSettings::mqXvid)) {
		while(time(NULL) - startTime < 10) Sleep(10*1000);
		startTime = time(NULL);

		CRss* rss = GetRssFeed(2, CString());
		if(rss) {
			CTransaction transaction(CNewzflow::Instance()->database);
			for(size_t i = 0; i < rss->items.GetCount(); i++) {
				CRssItem* item = rss->items[i];
				int quality = CSettings::mqXvid;
				InsertMovie(feedId, item, quality);
			}
			delete rss;
		}
	}

	rssWatcher->SetLastReply(_T(""));
	CNewzflow::Instance()->database->UpdateRssFeed(feedId, NULL);
	Util::PostMessageToMainWindow(Util::MSG_MOVIES_UPDATED);
}

void CProviderNzbMatrix::InsertMovie(int feedId, CRssItem* item, int quality)
{
	CRssWatcher* rssWatcher = CNewzflow::Instance()->rssWatcher;

	int itemId = CNewzflow::Instance()->database->InsertRssItem(feedId, item, quality); // returns 0 if item already existed
	if(itemId) {
		CString imdbId;
		int imdbStart = item->description.Find(_T("www.imdb.com/title/tt"));
		if(imdbStart > 0) {
			CString imdbId = item->description.Mid(imdbStart + _tcslen(_T("www.imdb.com/title/")), 9);
			rssWatcher->SetLastReply(item->title);
			rssWatcher->ProcessMovie(itemId, imdbId);
		}
	}
}
