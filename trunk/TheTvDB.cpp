/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TheTvDB.h"
#include "Newzflow.h"
#include "Settings.h"
#include "Util.h"

#include <algorithm>
#include <random>
#include "Compress.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

namespace TheTvDB
{

class CErrorHandler : public CComObjectRootEx<CComSingleThreadModel>, public ISAXErrorHandler
{
public:
	CErrorHandler()
	{
	}
	virtual ~CErrorHandler()
	{
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CErrorHandler)
	COM_INTERFACE_ENTRY(ISAXErrorHandler)
	END_COM_MAP()

	virtual HRESULT STDMETHODCALLTYPE error(/* [in] */ ISAXLocator *pLocator, /* [in] */ const wchar_t *pwchErrorMessage, /* [in] */ HRESULT hrErrorCode)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE fatalError(/* [in] */ ISAXLocator *pLocator, /* [in] */ const wchar_t *pwchErrorMessage, /* [in] */ HRESULT hrErrorCode)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ignorableWarning(/* [in] */ ISAXLocator *pLocator, /* [in] */ const wchar_t *pwchErrorMessage, /* [in] */ HRESULT hrErrorCode)
	{
		return S_OK;
	}
};

// CParser
//////////////////////////////////////////////////////////////////////////

class CParser : public CComObjectRootEx<CComSingleThreadModel>, public ISAXContentHandler
{
public:
	CParser()
	{
		m_pSeriesArray = NULL;
		m_pEpisodeArray = NULL;
		m_pSeries = NULL;
		m_pEpisode = NULL;
	}

	virtual ~CParser()
	{
	}

	void SetSeriesArray(CAtlArray<CSeries*>* seriesArray)
	{
		m_pSeriesArray = seriesArray;
	}
	void SetEpisodeArray(CAtlArray<CEpisode*>* episodeArray)
	{
		m_pEpisodeArray = episodeArray;
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CParser)
	COM_INTERFACE_ENTRY(ISAXContentHandler)
	END_COM_MAP()

	virtual HRESULT STDMETHODCALLTYPE putDocumentLocator(/* [in] */ ISAXLocator *pLocator)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE startDocument(void)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE endDocument(void)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE startPrefixMapping(/* [in] */ const wchar_t *pwchPrefix, /* [in] */ int cchPrefix, /* [in] */ const wchar_t *pwchUri, /* [in] */ int cchUri)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE endPrefixMapping(/* [in] */ const wchar_t *pwchPrefix, /* [in] */ int cchPrefix)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE startElement(/* [in] */ const wchar_t *pwchNamespaceUri, /* [in] */ int cchNamespaceUri, /* [in] */ const wchar_t *pwchLocalName, /* [in] */ int cchLocalName, /* [in] */ const wchar_t *pwchRawName, /* [in] */ int cchRawName, /* [in] */ ISAXAttributes *pAttributes)
	{
		CString localName(pwchLocalName, cchLocalName);
		curChars.Empty();
		if(!m_pSeries && localName == _T("Series")) {
			m_pSeries = new CSeries;
		} else if(!m_pSeries && localName == _T("Episode")) {
			m_pEpisode = new CEpisode;
		}
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE endElement(/* [in] */ const wchar_t *pwchNamespaceUri, /* [in] */ int cchNamespaceUri, /* [in] */ const wchar_t *pwchLocalName, /* [in] */ int cchLocalName, /* [in] */ const wchar_t *pwchRawName, /* [in] */ int cchRawName)
	{
		CString localName(pwchLocalName, cchLocalName);
		if(m_pSeries) {
			if(localName == _T("id")) {
				m_pSeries->id = _ttoi(curChars);
			} else if(localName == _T("SeriesName")) {
				m_pSeries->SeriesName = curChars;
			} else if(localName == _T("banner")) {
				m_pSeries->banner = curChars;
			} else if(localName == _T("poster")) {
				m_pSeries->poster = curChars;
			} else if(localName == _T("fanart")) {
				m_pSeries->fanart = curChars;
			} else if(localName == _T("Overview")) {
				m_pSeries->Overview = curChars;
			} else if(localName == _T("FirstAired")) {
				int year, month, day;
				_stscanf(curChars, _T("%d-%d-%d"), &year, &month, &day);
				m_pSeries->FirstAired.SetDate(year, month, day);
			} else if(localName == _T("Series")) {
				if(m_pSeriesArray)
					m_pSeriesArray->Add(m_pSeries);
				else
					delete m_pSeries;
				m_pSeries = NULL;
			}
		} else if(m_pEpisode) {
			if(localName == _T("id")) {
				m_pEpisode->id = _ttoi(curChars);
			} else if(localName == _T("SeasonNumber")) {
				m_pEpisode->SeasonNumber = _ttoi(curChars);
			} else if(localName == _T("EpisodeNumber")) {
				m_pEpisode->EpisodeNumber = _ttoi(curChars);
			} else if(localName == _T("EpisodeName")) {
				m_pEpisode->EpisodeName = curChars;
			} else if(localName == _T("Overview")) {
				m_pEpisode->Overview = curChars;
			} else if(localName == _T("FirstAired")) {
				int year, month, day;
				_stscanf(curChars, _T("%d-%d-%d"), &year, &month, &day);
				m_pEpisode->FirstAired.SetDate(year, month, day);
			} else if(localName == _T("Episode")) {
				if(m_pEpisodeArray && !m_pEpisode->EpisodeName.IsEmpty()) // skip episodes with empty name
					m_pEpisodeArray->Add(m_pEpisode);
				else
					delete m_pEpisode;
				m_pEpisode = NULL;
			}
		}
		curChars.Empty();

		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE characters(/* [in] */ const wchar_t *pwchChars, /* [in] */ int cchChars)
	{
		CString chars(pwchChars, cchChars);
		curChars += chars;

		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ignorableWhitespace(/* [in] */ const wchar_t *pwchChars, /* [in] */ int cchChars)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE processingInstruction(/* [in] */ const wchar_t *pwchTarget, /* [in] */ int cchTarget, /* [in] */ const wchar_t *pwchData, /* [in] */ int cchData)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE skippedEntity(/* [in] */ const wchar_t *pwchName, /* [in] */ int cchName)
	{
		return S_OK;
	}

private:
	CString curChars;

	CSeries* m_pSeries;
	CEpisode* m_pEpisode;

	CAtlArray<CSeries*>* m_pSeriesArray;
	CAtlArray<CEpisode*>* m_pEpisodeArray;
};

// CGetSeries
//////////////////////////////////////////////////////////////////////////

CGetSeries::CGetSeries()
{
	downloader.Init();
}

CGetSeries::~CGetSeries()
{
	Clear();
}

void CGetSeries::Clear()
{
	for(size_t i = 0; i < Series.GetCount(); i++) delete Series[i];
	Series.RemoveAll();
}

bool CGetSeries::Parse(const CString& path)
{
	bool retCode = false;

	// parse NZB file
	{
		CComPtr<ISAXXMLReader> pRdr;
		CoInitialize(NULL);
		HRESULT hr = pRdr.CoCreateInstance(__uuidof(SAXXMLReader30));
		if(SUCCEEDED(hr)) {
			CComObject<CParser>* content;
			CComObject<CParser>::CreateInstance(&content);
			hr = pRdr->putContentHandler(content);
			content->SetSeriesArray(&Series);
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
	}

	return retCode;
}

bool CGetSeries::Execute(const CString& seriesname)
{
	Clear();

	CString sUrl;
	sUrl.Format(_T("http://www.thetvdb.com/api/GetSeries.php?seriesname=%s"), seriesname);
	CString xmlFile = CNewzflow::Instance()->settings->GetAppDataDir() + _T("temp.xml");
	if(!downloader.Download(sUrl, xmlFile, CString(), NULL))
		return false;
	if(!Parse(xmlFile))
		return false;
	CFile::Delete(xmlFile);
	return true;
}

// CSeriesAll
//////////////////////////////////////////////////////////////////////////

CSeriesAll::CSeriesAll()
{
	downloader.Init();
}

CSeriesAll::~CSeriesAll()
{
	Clear();
}

void CSeriesAll::Clear()
{
	for(size_t i = 0; i < Series.GetCount(); i++) delete Series[i];
	Series.RemoveAll();
	for(size_t i = 0; i < Episodes.GetCount(); i++) delete Episodes[i];
	Episodes.RemoveAll();
}

bool CSeriesAll::Parse(const CString& path)
{
	bool retCode = false;

	// parse NZB file
	{
		CComPtr<ISAXXMLReader> pRdr;
		CoInitialize(NULL);
		HRESULT hr = pRdr.CoCreateInstance(__uuidof(SAXXMLReader30));
		if(SUCCEEDED(hr)) {
			CComObject<CParser>* content;
			CComObject<CParser>::CreateInstance(&content);
			hr = pRdr->putContentHandler(content);
			content->SetSeriesArray(&Series);
			content->SetEpisodeArray(&Episodes);
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
	}

	return retCode;
}

bool CSeriesAll::Execute(CMirrors& mirrors, int id, const CString& language)
{
	Clear();

	CString sUrl;
	sUrl.Format(_T("%s/api/%s/series/%d/all/%s.zip"), mirrors.GetZipMirror(), CAPI::apiKey, id, language);
	CString zipFile = CNewzflow::Instance()->settings->GetAppDataDir() + _T("temp.zip");
	if(!downloader.Download(sUrl, zipFile, CString(), NULL))
		return false;
	if(!Compress::UnZip(zipFile, CNewzflow::Instance()->settings->GetAppDataDir(), NULL)) {
		CFile::Delete(zipFile);
		return false;
	}
	CString xmlFile = CNewzflow::Instance()->settings->GetAppDataDir() + language + _T(".xml");
	CString actorsFile = CNewzflow::Instance()->settings->GetAppDataDir() + _T("actors.xml");
	CString bannersFile = CNewzflow::Instance()->settings->GetAppDataDir() + _T("banners.xml");
	if(!Parse(xmlFile))
		return false;
	CFile::Delete(xmlFile);
	CFile::Delete(actorsFile);
	CFile::Delete(bannersFile);
	CFile::Delete(zipFile);
	return true;
}


// CMirrorsParser
//////////////////////////////////////////////////////////////////////////

class CMirrorsParser : public CComObjectRootEx<CComSingleThreadModel>, public ISAXContentHandler
{
public:
	CMirrorsParser()
	{
		m_pOwner = NULL;
		m_pMirror = NULL;
	}

	virtual ~CMirrorsParser()
	{
	}

	void SetOwner(CMirrors* owner)
	{
		m_pOwner = owner;
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CMirrorsParser)
	COM_INTERFACE_ENTRY(ISAXContentHandler)
	END_COM_MAP()

	virtual HRESULT STDMETHODCALLTYPE putDocumentLocator(/* [in] */ ISAXLocator *pLocator)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE startDocument(void)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE endDocument(void)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE startPrefixMapping(/* [in] */ const wchar_t *pwchPrefix, /* [in] */ int cchPrefix, /* [in] */ const wchar_t *pwchUri, /* [in] */ int cchUri)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE endPrefixMapping(/* [in] */ const wchar_t *pwchPrefix, /* [in] */ int cchPrefix)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE startElement(/* [in] */ const wchar_t *pwchNamespaceUri, /* [in] */ int cchNamespaceUri, /* [in] */ const wchar_t *pwchLocalName, /* [in] */ int cchLocalName, /* [in] */ const wchar_t *pwchRawName, /* [in] */ int cchRawName, /* [in] */ ISAXAttributes *pAttributes)
	{
		CString localName(pwchLocalName, cchLocalName);
		curChars.Empty();
		if(!m_pMirror && localName == _T("Mirror")) {
			m_pMirror = new CMirror;
			m_pOwner->Mirrors.Add(m_pMirror);
		}
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE endElement(/* [in] */ const wchar_t *pwchNamespaceUri, /* [in] */ int cchNamespaceUri, /* [in] */ const wchar_t *pwchLocalName, /* [in] */ int cchLocalName, /* [in] */ const wchar_t *pwchRawName, /* [in] */ int cchRawName)
	{
		CString localName(pwchLocalName, cchLocalName);
		if(m_pMirror && localName == _T("mirrorpath")) {
			m_pMirror->mirrorpath = curChars;
		} else if(m_pMirror && localName == _T("typemask")) {
			m_pMirror->typemask = _ttoi(curChars);
		} else if(m_pMirror && localName == _T("Mirror")) {
			m_pMirror = NULL;
		}
		curChars.Empty();

		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE characters(/* [in] */ const wchar_t *pwchChars, /* [in] */ int cchChars)
	{
		CString chars(pwchChars, cchChars);
		curChars += chars;

		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ignorableWhitespace(/* [in] */ const wchar_t *pwchChars, /* [in] */ int cchChars)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE processingInstruction(/* [in] */ const wchar_t *pwchTarget, /* [in] */ int cchTarget, /* [in] */ const wchar_t *pwchData, /* [in] */ int cchData)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE skippedEntity(/* [in] */ const wchar_t *pwchName, /* [in] */ int cchName)
	{
		return S_OK;
	}

private:
	CString curChars;
	CMirror* m_pMirror;

	CMirrors* m_pOwner;
};

// CMirrors
//////////////////////////////////////////////////////////////////////////

CMirrors::CMirrors()
{
	downloader.Init();
}

CMirrors::~CMirrors()
{
	Clear();
}

void CMirrors::Clear()
{
	for(size_t i = 0; i < Mirrors.GetCount(); i++) delete Mirrors[i];
	Mirrors.RemoveAll();
}

bool CMirrors::Parse(const CString& path)
{
	bool retCode = false;

	// parse NZB file
	{
		CComPtr<ISAXXMLReader> pRdr;
		CoInitialize(NULL);
		HRESULT hr = pRdr.CoCreateInstance(__uuidof(SAXXMLReader30));
		if(SUCCEEDED(hr)) {
			CComObject<CMirrorsParser>* content;
			CComObject<CMirrorsParser>::CreateInstance(&content);
			hr = pRdr->putContentHandler(content);
			content->SetOwner(this);
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
	}

	return retCode;
}

bool CMirrors::Execute()
{
	Clear();

	CString sUrl;
	sUrl.Format(_T("http://www.thetvdb.com/api/%s/mirrors.xml"), CAPI::apiKey);
	CString xmlFile = CNewzflow::Instance()->settings->GetAppDataDir() + _T("temp.xml");
	if(!downloader.Download(sUrl, xmlFile, CString(), NULL))
		return false;
	if(!Parse(xmlFile))
		return false;
	CFile::Delete(xmlFile);
	return true;
}

CString CMirrors::GetXmlMirror()
{
	return GetMirror(1);
}

CString CMirrors::GetBannerMirror()
{
	return GetMirror(2);
}

CString CMirrors::GetZipMirror()
{
	return GetMirror(4);
}

CString CMirrors::GetMirror(int type)
{
	CAtlArray<CMirror*> mirrors;
	for(size_t i = 0; i < Mirrors.GetCount(); i++) {
		if(Mirrors[i]->typemask & type)
			mirrors.Add(Mirrors[i]);
	}

	if(mirrors.IsEmpty())
		return CString();

	std::tr1::minstd_rand gen;
	std::tr1::uniform_int<int> dist(0, mirrors.GetCount()-1);
	gen.seed((unsigned int)time(NULL));

	return mirrors[dist(gen)]->mirrorpath;
}

// CAPI
//////////////////////////////////////////////////////////////////////////

/*static*/ const CString CAPI::apiKey = _T("E13C9BDFF5677364");

CAPI::CAPI()
{
	mirrors.Execute();
}

CAPI::~CAPI()
{
}

CGetSeries* CAPI::GetSeries(const CString& seriesname)
{
	CGetSeries* getSeries = new CGetSeries;
	if(getSeries->Execute(seriesname))
		return getSeries;

	delete getSeries;
	return NULL;
}

CSeriesAll* CAPI::SeriesAll(int id, const CString& language /*= _T("en")*/)
{
	CSeriesAll* seriesAll = new CSeriesAll;
	if(seriesAll->Execute(mirrors, id, language))
		return seriesAll;

	delete seriesAll;
	return NULL;
}

} // namespace TheTvDB