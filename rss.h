#pragma once

class CRssEnclosure
{
public:
	CRssEnclosure() {
		length = 0;
	}

	// <enclosure>
	CString url, type;
	__int64 length;
};

class CRssItem
{
public:
	CRssItem() {
		enclosure = NULL;
	}
	~CRssItem() {
		delete enclosure;
	}

	// <item>
	CString title, link, description;
	CRssEnclosure* enclosure;
	CString category;
	COleDateTime pubDate;
};

class CRss
{
public:
	CRss() {
		ttl = 0;
	}
	~CRss() {
		for(size_t i = 0; i < items.GetCount(); i++) delete items[i];
	}

	bool Parse(const CString& path);

	// <channel>
	CString title, link, description;
	int ttl;
	COleDateTime pubDate;

	CAtlArray<CRssItem*> items;
};