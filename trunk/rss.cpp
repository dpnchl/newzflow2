/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rss.h"
#include "Newzflow.h"
#include "Settings.h"
#include "Util.h"

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

namespace {

class CRssParser : public CComObjectRootEx<CComSingleThreadModel>, public ISAXContentHandler
{
public:
	CRssParser();
	virtual ~CRssParser();

	void SetRss(CRss* _rss) { rss = _rss; }

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CRssParser)
	COM_INTERFACE_ENTRY(ISAXContentHandler)
	END_COM_MAP()

	virtual HRESULT STDMETHODCALLTYPE putDocumentLocator( 
		/* [in] */ ISAXLocator *pLocator);

	virtual HRESULT STDMETHODCALLTYPE startDocument( void);

	virtual HRESULT STDMETHODCALLTYPE endDocument( void);

	virtual HRESULT STDMETHODCALLTYPE startPrefixMapping( 
		/* [in] */ const wchar_t *pwchPrefix,
		/* [in] */ int cchPrefix,
		/* [in] */ const wchar_t *pwchUri,
		/* [in] */ int cchUri);

	virtual HRESULT STDMETHODCALLTYPE endPrefixMapping( 
		/* [in] */ const wchar_t *pwchPrefix,
		/* [in] */ int cchPrefix);

	virtual HRESULT STDMETHODCALLTYPE startElement( 
		/* [in] */ const wchar_t *pwchNamespaceUri,
		/* [in] */ int cchNamespaceUri,
		/* [in] */ const wchar_t *pwchLocalName,
		/* [in] */ int cchLocalName,
		/* [in] */ const wchar_t *pwchRawName,
		/* [in] */ int cchRawName,
		/* [in] */ ISAXAttributes *pAttributes);

	virtual HRESULT STDMETHODCALLTYPE endElement( 
		/* [in] */ const wchar_t *pwchNamespaceUri,
		/* [in] */ int cchNamespaceUri,
		/* [in] */ const wchar_t *pwchLocalName,
		/* [in] */ int cchLocalName,
		/* [in] */ const wchar_t *pwchRawName,
		/* [in] */ int cchRawName);

	virtual HRESULT STDMETHODCALLTYPE characters( 
		/* [in] */ const wchar_t *pwchChars,
		/* [in] */ int cchChars);

	virtual HRESULT STDMETHODCALLTYPE ignorableWhitespace( 
		/* [in] */ const wchar_t *pwchChars,
		/* [in] */ int cchChars);

	virtual HRESULT STDMETHODCALLTYPE processingInstruction( 
		/* [in] */ const wchar_t *pwchTarget,
		/* [in] */ int cchTarget,
		/* [in] */ const wchar_t *pwchData,
		/* [in] */ int cchData);

	virtual HRESULT STDMETHODCALLTYPE skippedEntity( 
		/* [in] */ const wchar_t *pwchName,
		/* [in] */ int cchName);

private:
	CRss* rss;
	CRssItem* curItem;
	CString curTag;
};

class CErrorHandler : public CComObjectRootEx<CComSingleThreadModel>, public ISAXErrorHandler
{
public:
	CErrorHandler();
	virtual ~CErrorHandler();

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CErrorHandler)
	COM_INTERFACE_ENTRY(ISAXErrorHandler)
	END_COM_MAP()

	virtual HRESULT STDMETHODCALLTYPE error( 
		/* [in] */ ISAXLocator *pLocator,
		/* [in] */ const wchar_t *pwchErrorMessage,
		/* [in] */ HRESULT hrErrorCode);

	virtual HRESULT STDMETHODCALLTYPE fatalError( 
		/* [in] */ ISAXLocator *pLocator,
		/* [in] */ const wchar_t *pwchErrorMessage,
		/* [in] */ HRESULT hrErrorCode);

	virtual HRESULT STDMETHODCALLTYPE ignorableWarning( 
		/* [in] */ ISAXLocator *pLocator,
		/* [in] */ const wchar_t *pwchErrorMessage,
		/* [in] */ HRESULT hrErrorCode);
};

CRssParser::CRssParser()
{
	rss = NULL;
	curItem = NULL;
}

CRssParser::~CRssParser()
{
}

HRESULT STDMETHODCALLTYPE CRssParser::startElement( 
	/* [in] */ const wchar_t *pwchNamespaceUri,
	/* [in] */ int cchNamespaceUri,
	/* [in] */ const wchar_t *pwchLocalName,
	/* [in] */ int cchLocalName,
	/* [in] */ const wchar_t *pwchRawName,
	/* [in] */ int cchRawName,
	/* [in] */ ISAXAttributes *pAttributes)
{
	ASSERT(rss);
	CString localName(pwchLocalName, cchLocalName);
	if(!curItem && localName == _T("item")) {
		curItem = new CRssItem;
		curItem->pubDate = rss->pubDate; // default to rss's pubDate
		rss->items.Add(curItem);
	} else if(curItem && localName == _T("enclosure")) {
		curItem->enclosure = new CRssEnclosure;
		const wchar_t* pwchURL;
		int cchURL;
		VERIFY(SUCCEEDED(pAttributes->getValueFromName(L"", 0, L"url", wcslen(L"url"), &pwchURL, &cchURL)));
		curItem->enclosure->url = CString(pwchURL, cchURL);
		const wchar_t* pwchLength;
		int cchLength;
		VERIFY(SUCCEEDED(pAttributes->getValueFromName(L"", 0, L"length", wcslen(L"length"), &pwchLength, &cchLength)));
		curItem->enclosure->length = _tcstoui64(CString(pwchLength, cchLength), NULL, 10);
		const wchar_t* pwchType;
		int cchType;
		VERIFY(SUCCEEDED(pAttributes->getValueFromName(L"", 0, L"type", wcslen(L"type"), &pwchType, &cchType)));
		curItem->enclosure->type = CString(pwchType, cchType);
	} else if(curItem && localName == _T("category")) {
		if(!curItem->category.IsEmpty()) curItem->category += _T(", ");
	} else {
		curTag = localName;
	}
	return S_OK;
}

// Note: the SAX parser issues multiple "characters" calls, when entities are embedded
// e.g. hello&amp;world will result in calls "hello", "&" and "world", so make sure to concat the strings
HRESULT STDMETHODCALLTYPE CRssParser::characters( 
	/* [in] */ const wchar_t *pwchChars,
	/* [in] */ int cchChars)
{
	ASSERT(rss);
	CString chars(pwchChars, cchChars);
	if(curItem) {
		if(curTag == _T("title")) {
			curItem->title += chars;
		} else if(curTag == _T("link")) {
			curItem->link += chars;
		} else if(curTag == _T("description")) {
			curItem->description += chars;
		} else if(curTag == _T("category")) {
			curItem->category += chars;
		} else if(curTag == _T("pubDate")) {
			SYSTEMTIME sTime;
			if(WinHttpTimeToSystemTime(CStringW(chars), &sTime))
				curItem->pubDate = sTime;
		}
	} else {
		if(curTag == _T("title")) {
			rss->title += chars;
		} else if(curTag == _T("link")) {
			rss->link += chars;
		} else if(curTag == _T("description")) {
			rss->description += chars;
		} else if(curTag == _T("ttl")) {
			rss->ttl += _ttoi(chars);
		} else if(curTag == _T("pubDate")) {
			SYSTEMTIME sTime;
			if(WinHttpTimeToSystemTime(CStringW(chars), &sTime))
				rss->pubDate = sTime;
		}
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CRssParser::endElement( 
	/* [in] */ const wchar_t *pwchNamespaceUri,
	/* [in] */ int cchNamespaceUri,
	/* [in] */ const wchar_t *pwchLocalName,
	/* [in] */ int cchLocalName,
	/* [in] */ const wchar_t *pwchRawName,
	/* [in] */ int cchRawName)
{
	ASSERT(rss);
	CString localName(pwchLocalName, cchLocalName);
	if(curItem && localName == _T("item")) {
		curItem = NULL;
	}
	curTag.Empty();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CRssParser::startDocument()
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CRssParser::putDocumentLocator( 
	/* [in] */ ISAXLocator *pLocator
	)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CRssParser::endDocument( void)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CRssParser::startPrefixMapping( 
	/* [in] */ const wchar_t *pwchPrefix,
	/* [in] */ int cchPrefix,
	/* [in] */ const wchar_t *pwchUri,
	/* [in] */ int cchUri)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CRssParser::endPrefixMapping( 
	/* [in] */ const wchar_t *pwchPrefix,
	/* [in] */ int cchPrefix)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CRssParser::ignorableWhitespace( 
	/* [in] */ const wchar_t *pwchChars,
	/* [in] */ int cchChars)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CRssParser::processingInstruction( 
	/* [in] */ const wchar_t *pwchTarget,
	/* [in] */ int cchTarget,
	/* [in] */ const wchar_t *pwchData,
	/* [in] */ int cchData)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CRssParser::skippedEntity( 
	/* [in] */ const wchar_t *pwchVal,
	/* [in] */ int cchVal)
{
	CString chars(pwchVal, cchVal);


	return S_OK;
}

CErrorHandler::CErrorHandler()
{
}

CErrorHandler::~CErrorHandler()
{
}

HRESULT STDMETHODCALLTYPE CErrorHandler::error( 
	/* [in] */ ISAXLocator *pLocator,
	/* [in] */ const wchar_t * pwchErrorMessage,
	/* [in] */ HRESULT errCode)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CErrorHandler::fatalError( 
	/* [in] */ ISAXLocator *pLocator,
	/* [in] */ const wchar_t * pwchErrorMessage,
	/* [in] */ HRESULT errCode)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CErrorHandler::ignorableWarning( 
	/* [in] */ ISAXLocator *pLocator,
	/* [in] */ const wchar_t * pwchErrorMessage,
	/* [in] */ HRESULT errCode)
{
	return S_OK;
}

} // namespace

bool CRss::Parse(const CString& path, const CString& url)
{
	bool retCode = false;

	// parse NZB file
	{
		CComPtr<ISAXXMLReader> pRdr;
		CoInitialize(NULL);
		HRESULT hr = pRdr.CoCreateInstance(__uuidof(SAXXMLReader30));
		if(SUCCEEDED(hr)) {
			CComObject<CRssParser>* content;
			CComObject<CRssParser>::CreateInstance(&content);
			content->SetRss(this);
			hr = pRdr->putContentHandler(content);
			if(SUCCEEDED(hr)) {
				CComObject<CErrorHandler>* error;
				CComObject<CErrorHandler>::CreateInstance(&error);
				hr = pRdr->putErrorHandler(error);
				if(SUCCEEDED(hr)) {
					hr = pRdr->parseURL(path);
					if(SUCCEEDED(hr)) {
						retCode = true;
					}
				}
			}
		}
		CoUninitialize();
	}

	// Post-processing
	if(retCode) {
		CString urlLC(url);
		urlLC.MakeLower();
		if(urlLC.Find(_T("nzbmatrix")))
			FixupNzbMatrix();

	}

	return retCode;
}

void CRss::FixupNzbMatrix()
{
	using std::tr1::tregex; using std::tr1::tcmatch; using std::tr1::regex_search; using std::tr1::regex_match;
	tregex reDate(_T("([0-9]{4})-([0-9]{2})-([0-9]{2}) ([0-9]{2}):([0-9]{2}):([0-9]{2})"));
	tregex reSize(_T("([0-9]{1,4}(\\.[0-9]+)? GB)"));
	tcmatch match;

	static const __int64 _2GB = 2LL*1024LL*1024LL*1024LL;
	static const __int64 _4GB = 4LL*1024LL*1024LL*1024LL;

	for(size_t i = 0; i < items.GetCount(); i++) {
		CRssItem* item = items[i];

		// NzbMatrix omits item/pubDate, so parse from item/description
		if(regex_search((const TCHAR*)item->description, match, reDate)) {
			item->pubDate.SetDateTime(
				_ttoi(CString(match[1].first, match[1].length())),
				_ttoi(CString(match[2].first, match[2].length())),
				_ttoi(CString(match[3].first, match[3].length())),
				_ttoi(CString(match[4].first, match[4].length())),
				_ttoi(CString(match[5].first, match[5].length())),
				_ttoi(CString(match[6].first, match[6].length()))
			);
		}

		// NzbMatrix's item/enclosure/length is truncated to 32 bit, so reconstruct real size from item/description
		if(item->enclosure->length > 0 && item->enclosure->length < _4GB && regex_search((const TCHAR*)item->description, match, reSize)) {
			__int64 size = Util::ParseSize(CString(match[1].first, match[1].length()));
			// just keep adding 4GB until we approximately reach the GB size matched from item/description (+/- 2GB)
			while(item->enclosure->length + _2GB < size)
				item->enclosure->length += _4GB;
		}
	}
}