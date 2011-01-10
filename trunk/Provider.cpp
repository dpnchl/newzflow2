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
	// Process RSS Feeds
	QRssFeedsToRefresh* rss = CNewzflow::Instance()->database->GetRssFeedsToRefresh(_T("nzbmatrix.com"));
	while(rss->GetRow()) {
		CString s; s.Format(_T("CProviderNzbMatrix::Process(%s)\n"), rss->GetSpecial()); Util::Print(s);
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
		if(!feedId) {
			q.Prepare(_T("INSERT INTO RssFeeds (title, update_interval, provider, special) VALUES (?, ?, ?, ?)"), _T("nzbmatrix.com Movies"), 25, _T("nzbmatrix.com"), _T("Movies"));
			feedId = q.ExecuteInsert();
		}
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

void CProviderNzbMatrix::ProcessMovies(int feedId)
{
	CString rssFile = CNewzflow::Instance()->settings->GetAppDataDir() + _T("temp.rss");

	CSettings* settings = CNewzflow::Instance()->settings;
	unsigned qualityMask = _ttoi(settings->GetIni(_T("Movies"), _T("QualityMask"), _T("0")));

	unsigned qmMKV = qualityMask & ((1 << CSettings::mqMKV1080p) | (1 << CSettings::mqMKV720p));
	if(qmMKV) {
		CString term;
		if(qmMKV == ((1 << CSettings::mqMKV1080p) | (1 << CSettings::mqMKV720p)))
			term = _T("(1080p,720p)");
		else if(qmMKV == (1 << CSettings::mqMKV1080p))
			term = _T("1080p");
		else if(qmMKV == (1 << CSettings::mqMKV720p))
			term = _T("720p");

		CString sUrl;
		sUrl.Format(_T("http://rss.nzbmatrix.com/rss.php?page=download&username=%s&apikey=%s&subcat=42&scenename=1&term=%s"), settings->GetIni(_T("nzbmatrix.com"), _T("User")), settings->GetIni(_T("nzbmatrix.com"), _T("ApiKey")), term);

		if(downloader.Download(sUrl, rssFile, CString(), NULL)) {
			CRss rss;
			if(rss.Parse(rssFile, sUrl)) {
				CTransaction transaction(CNewzflow::Instance()->database);
				for(size_t i = 0; i < rss.items.GetCount(); i++) {
					CRssItem* item = rss.items[i];
					int quality = -1;
					if(item->title.Find(_T("1080p")) >= 0)
						quality = CSettings::mqMKV1080p;
					else if(item->title.Find(_T("720p")) >= 0)
						quality = CSettings::mqMKV720p;
					int itemId = CNewzflow::Instance()->database->InsertRssItem(feedId, item, quality); // returns 0 if item already existed
					if(itemId) {
						CString imdbId;
						int imdbStart = item->description.Find(_T("www.imdb.com/title/tt"));
						if(imdbStart > 0) {
							CString imdbId = item->description.Mid(imdbStart + _tcslen(_T("www.imdb.com/title/")), 9);
							CString s; s.Format(_T("%s: ProcessMovie(%s %s (%d))\n"), __FUNCTIONW__, item->title, imdbId, itemId); Util::Print(s);
							CNewzflow::Instance()->rssWatcher->ProcessMovie(itemId, imdbId);
						}
					}
				}
			} else {
				CString sError;
				sError.Format(_T("%s:Error parsing RSS feed %s\n"), __FUNCTIONW__, sUrl);
			}
		} else {
			CString sError;
			sError.Format(_T("%s:Error downloading RSS feed %s\n"), __FUNCTIONW__, sUrl);
		}
	}
	CFile::Delete(rssFile);

	CNewzflow::Instance()->database->UpdateRssFeed(feedId, NULL);
	Util::PostMessageToMainWindow(Util::MSG_MOVIES_UPDATED);
}