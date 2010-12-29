#pragma once

// see http://api.themoviedb.org/2.1

#include "HttpDownloader.h"

namespace TheMovieDB
{

class CImage
{
public:
	CString id, type, size, url;
};

class CPerson
{
public:
	CPerson()
	{
		id = order = cast_id = 0;
	}
	~CPerson()
	{
		for(size_t i = 0; i < images.GetCount(); i++) delete images[i];
	}

	int id;
	CString name, department, job;
	int order, cast_id;
	CAtlArray<CImage*> images;
};

class CMovie
{
public:
	CMovie()
	{
		id = 0;
	}
	~CMovie()
	{
		for(size_t i = 0; i < images.GetCount(); i++) delete images[i];
	}

	int id;
	CString name;
	CString overview;
	CAtlArray<CImage*> images;
	CAtlArray<CPerson*> cast;
};

class CGetMovie
{
public:
	CGetMovie();
	~CGetMovie();

	void Clear();
	bool Execute(const CString& imdbId);
	bool Execute(int tmdbId);

	CAtlArray<CMovie*> Movies;

protected:
	bool Parse(const CString& path);

	CHttpDownloader downloader;
};

class CAPI
{
public:
	CAPI();
	~CAPI();

	CGetMovie* GetMovie(const CString& imdbId);
	CGetMovie* GetMovie(int tmdbId);

	static const CString apiKey;
};

}