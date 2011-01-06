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

	bool Parse(const CString& path, const CString& url); // parse rss file from "path"; "url" is used to determine if post-processing (fixup) is necessary

	// <channel>
	CString title, link, description;
	int ttl;
	COleDateTime pubDate;

	CAtlArray<CRssItem*> items;

protected:
	void FixupNzbMatrix();
};