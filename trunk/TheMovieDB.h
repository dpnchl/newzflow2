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

class CImageArray : public CAtlArray<CImage*>
{
public:
	~CImageArray()
	{
		for(size_t i = 0; i < GetCount(); i++) delete GetAt(i);
	}

	int Find(LPCTSTR type, LPCTSTR size, int lastIndex = -1)
	{
		for(size_t i = lastIndex+1; i < GetCount(); i++) {
			if((type == NULL || GetAt(i)->type == type) && (size == NULL || GetAt(i)->size == size))
				return i;
		}
		return -1;
	}
};

class CPerson
{
public:
	CPerson()
	{
		id = order = cast_id = 0;
	}

	int id;
	CString name, department, job;
	int order, cast_id;
	CImageArray images;
};

class CPersonArray : public CAtlArray<CPerson*>
{
public:
	~CPersonArray()
	{
		for(size_t i = 0; i < GetCount(); i++) delete GetAt(i);
	}

	int Find(LPCTSTR department, LPCTSTR job, int lastIndex = -1)
	{
		for(size_t i = lastIndex+1; i < GetCount(); i++) {
			if((department == NULL || GetAt(i)->department == department) && (job == NULL || GetAt(i)->job == job))
				return i;
		}
		return -1;
	}
};

class CMovie
{
public:
	CMovie()
	{
		id = 0;
	}

	int id;
	CString name;
	CString overview;
	CImageArray images;
	CPersonArray cast;
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

class CGetPerson
{
public:
	CGetPerson();
	~CGetPerson();

	void Clear();
	bool Execute(int tmdbId);

	CAtlArray<CPerson*> Persons;

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
	CGetPerson* GetPerson(int tmdbId);

	static const CString apiKey;
};

}