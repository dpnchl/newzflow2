#pragma once

#include "rss.h"
#include "HttpDownloader.h"

class IProvider
{
public:
	IProvider();
	virtual ~IProvider();

	virtual bool Process() = 0;
	virtual void OnSettingsChanged() = 0;
};

class CProviderNzbMatrix : public IProvider
{
public:
	CProviderNzbMatrix();
	~CProviderNzbMatrix();

	bool Process();
	void OnSettingsChanged();

protected:
	void ProcessMovies(int feedId);

	CHttpDownloader downloader;
};
