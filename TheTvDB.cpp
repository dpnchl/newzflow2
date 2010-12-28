/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TheTvDB.h"
#include "Newzflow.h"
#include "Settings.h"
#include "Util.h"

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

namespace TheTvDB
{

class CGetSeriesParser : public CComObjectRootEx<CComSingleThreadModel>, public ISAXContentHandler
{
public:
	CGetSeriesParser()
	{
		m_pOwner = NULL;
		m_pSeries = NULL;
	}

	virtual ~CGetSeriesParser()
	{
	}

	void SetOwner(CGetSeries* owner)
	{
		m_pOwner = owner;
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CGetSeriesParser)
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
			m_pOwner->Data.Add(m_pSeries);
		}
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE endElement(/* [in] */ const wchar_t *pwchNamespaceUri, /* [in] */ int cchNamespaceUri, /* [in] */ const wchar_t *pwchLocalName, /* [in] */ int cchLocalName, /* [in] */ const wchar_t *pwchRawName, /* [in] */ int cchRawName)
	{
		CString localName(pwchLocalName, cchLocalName);
		if(m_pSeries && localName == _T("seriesid")) {
			m_pSeries->seriesid = _ttoi(curChars);
		} else if(m_pSeries && localName == _T("SeriesName")) {
			m_pSeries->SeriesName = curChars;
		} else if(m_pSeries && localName == _T("banner")) {
			m_pSeries->banner = curChars;
		} else if(m_pSeries && localName == _T("Overview")) {
			m_pSeries->Overview = curChars;
		} else if(m_pSeries && localName == _T("FirstAired")) {
			int year, month, day;
			_stscanf(curChars, _T("%d-%d-%d"), &year, &month, &day);
			m_pSeries->FirstAired.SetDate(year, month, day);
		} else if(m_pSeries && localName == _T("Series")) {
			m_pSeries = NULL;
		}
		curTag.Empty();

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
	CString curTag;
	CString curChars;
	CSeries* m_pSeries;

	CGetSeries* m_pOwner;
};

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

bool CGetSeries::Parse(const CString& path)
{
	bool retCode = false;

	// parse NZB file
	{
		CComPtr<ISAXXMLReader> pRdr;
		CoInitialize(NULL);
		HRESULT hr = pRdr.CoCreateInstance(__uuidof(SAXXMLReader30));
		if(SUCCEEDED(hr)) {
			CComObject<CGetSeriesParser>* content;
			CComObject<CGetSeriesParser>::CreateInstance(&content);
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

} // namespace TheTvDB