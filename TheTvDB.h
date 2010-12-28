#pragma once

// see http://thetvdb.com/wiki/index.php?title=Programmers_API

#include "HttpDownloader.h"

namespace TheTvDB
{

class CSeries
{
public:
	CSeries()
	{
		id = 0;
	}

	int id;
	CString SeriesName;
	CString banner, fanart, poster;
	CString Overview;
	COleDateTime FirstAired;
};

class CEpisode
{
public:
	CEpisode()
	{
		id = 0;
		SeasonNumber = 0;
		EpisodeNumber = 0;
	}

	int id;
	int SeasonNumber, EpisodeNumber;
	CString EpisodeName;
	CString Overview;
	COleDateTime FirstAired;
};

class CGetSeries
{
public:
	CGetSeries();
	~CGetSeries();

	void Clear();
	bool Execute(const CString& seriesname);

	CAtlArray<CSeries*> Series;

protected:
	bool Parse(const CString& path);

	CHttpDownloader downloader;
};

class CSeriesAll
{
public:
	CSeriesAll();
	~CSeriesAll();

	void Clear();
	bool Execute(class CMirrors& mirrors, int id, const CString& language = _T("en"));

	CAtlArray<CSeries*> Series;
	CAtlArray<CEpisode*> Episodes;

protected:
	bool Parse(const CString& path);

	CHttpDownloader downloader;
};

class CMirror
{
public:
	CMirror() 
	{
		typemask = 0;
	}

	CString mirrorpath;
	int typemask;
};

class CMirrors
{
public:
	CMirrors();
	~CMirrors();

	void Clear();
	bool Execute();

	CString GetXmlMirror();
	CString GetBannerMirror();
	CString GetZipMirror();

protected:
	bool Parse(const CString& path);
	CString GetMirror(int type);

	CAtlArray<CMirror*> Mirrors;
	CHttpDownloader downloader;

	friend class CMirrorsParser;
};

class CAPI
{
public:
	CAPI();
	~CAPI();

	CGetSeries* GetSeries(const CString& seriesname);
	CSeriesAll* SeriesAll(int id, const CString& language = _T("en"));

	CMirrors mirrors;
	static const CString apiKey;
};

}