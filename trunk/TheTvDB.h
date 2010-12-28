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
		seriesid = 0;
	}

	int seriesid;
	CString SeriesName;
	CString banner;
	CString Overview;
	COleDateTime FirstAired;
};

class CGetSeries
{
public:
	CGetSeries() 
	{
		downloader.Init();
	}
	~CGetSeries() 
	{
		Clear();
	}

	void Clear()
	{
		for(size_t i = 0; i < Data.GetCount(); i++) delete Data[i];
		Data.RemoveAll();
	}

	bool Execute(const CString& seriesname);

	CAtlArray<CSeries*> Data;

protected:
	bool Parse(const CString& path);

	CHttpDownloader downloader;
};

}