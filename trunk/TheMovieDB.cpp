/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TheMovieDB.h"
#include "Newzflow.h"
#include "Settings.h"
#include "Util.h"

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

namespace TheMovieDB
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
		m_pPersonArray = NULL;
		m_pMovieArray = NULL;
		m_pMovie = NULL;
		m_pPerson = NULL;
	}

	virtual ~CParser()
	{
	}

	void SetPersonArray(CAtlArray<CPerson*>* personArray)
	{
		m_pPersonArray = personArray;
	}
	void SetMovieArray(CAtlArray<CMovie*>* movieArray)
	{
		m_pMovieArray = movieArray;
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

	CString getAttribute(ISAXAttributes *pAttributes, const wchar_t* name)
	{
		const wchar_t* pwchValue;
		int cchValue;
		if(SUCCEEDED(pAttributes->getValueFromName(L"", 0, name, wcslen(name), &pwchValue, &cchValue)))
			return CString(pwchValue, cchValue);
		else
			return CString();
	}

	virtual HRESULT STDMETHODCALLTYPE startElement(/* [in] */ const wchar_t *pwchNamespaceUri, /* [in] */ int cchNamespaceUri, /* [in] */ const wchar_t *pwchLocalName, /* [in] */ int cchLocalName, /* [in] */ const wchar_t *pwchRawName, /* [in] */ int cchRawName, /* [in] */ ISAXAttributes *pAttributes)
	{
		CString localName(pwchLocalName, cchLocalName);
		curChars.Empty();
		if(!m_pMovie && localName == _T("movie")) {
			m_pMovie = new CMovie;
		}
		if(localName == _T("person")) {
			m_pPerson = new CPerson;
			m_pPerson->id = _ttoi(getAttribute(pAttributes, L"id"));
			m_pPerson->name = getAttribute(pAttributes, L"name");
			m_pPerson->job = getAttribute(pAttributes, L"job");
			m_pPerson->department = getAttribute(pAttributes, L"department");
			m_pPerson->order = _ttoi(getAttribute(pAttributes, L"order"));
			m_pPerson->cast_id = _ttoi(getAttribute(pAttributes, L"cast_id"));
		} 
		if(localName == _T("image")) {
			CImage* pImage = new CImage;
			pImage->id = getAttribute(pAttributes, L"id");
			pImage->type = getAttribute(pAttributes, L"type");
			pImage->url = getAttribute(pAttributes, L"url");
			pImage->size = getAttribute(pAttributes, L"size");

			if(m_pPerson)
				m_pPerson->images.Add(pImage);
			else if(m_pMovie)
				m_pMovie->images.Add(pImage);
			else
				delete pImage;
		}
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE endElement(/* [in] */ const wchar_t *pwchNamespaceUri, /* [in] */ int cchNamespaceUri, /* [in] */ const wchar_t *pwchLocalName, /* [in] */ int cchLocalName, /* [in] */ const wchar_t *pwchRawName, /* [in] */ int cchRawName)
	{
		CString localName(pwchLocalName, cchLocalName);
		if(m_pPerson) {
			if(localName == _T("id")) {
				m_pPerson->id = _ttoi(curChars);
			} else if(localName == _T("name")) {
				m_pPerson->name = curChars;
			} else if(localName == _T("person")) {
				if(m_pPersonArray)
					m_pPersonArray->Add(m_pPerson);
				else if(m_pMovie)
					m_pMovie->cast.Add(m_pPerson);
				else
					delete m_pPerson;
				m_pPerson = NULL;
			}
		} else if(m_pMovie) {
			if(localName == _T("id")) {
				m_pMovie->id = _ttoi(curChars);
			} else if(localName == _T("name")) {
				m_pMovie->name = curChars;
			} else if(localName == _T("overview")) {
				m_pMovie->overview = curChars;
			} else if(localName == _T("released")) {
				//int year, month, day;
				//_stscanf(curChars, _T("%d-%d-%d"), &year, &month, &day);
				//m_pMovie->FirstAired.SetDate(year, month, day);
			} else if(localName == _T("movie")) {
				if(m_pMovieArray)
					m_pMovieArray->Add(m_pMovie);
				else
					delete m_pMovie;
				m_pMovie = NULL;
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

	CMovie* m_pMovie;
	CPerson* m_pPerson;

	CAtlArray<CPerson*>* m_pPersonArray;
	CAtlArray<CMovie*>* m_pMovieArray;
};

// CGetMovie
//////////////////////////////////////////////////////////////////////////

CGetMovie::CGetMovie()
{
	downloader.Init();
}

CGetMovie::~CGetMovie()
{
	Clear();
}

void CGetMovie::Clear()
{
	for(size_t i = 0; i < Movies.GetCount(); i++) delete Movies[i];
	Movies.RemoveAll();
}

bool CGetMovie::Parse(const CString& path)
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
			content->SetMovieArray(&Movies);
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

bool CGetMovie::Execute(const CString& imdbId)
{
	Clear();

	CString sUrl;
	sUrl.Format(_T("http://api.themoviedb.org/2.1/Movie.imdbLookup/en/xml/%s/%s"), CAPI::apiKey, imdbId);
	CString xmlFile = CNewzflow::Instance()->settings->GetAppDataDir() + _T("temp.xml");
	if(!downloader.Download(sUrl, xmlFile, CString(), NULL))
		return false;
	if(!Parse(xmlFile))
		return false;
	CFile::Delete(xmlFile);
	if(!Movies.IsEmpty())
		return Execute(Movies[0]->id);
	else
		return false;
}

bool CGetMovie::Execute(int tmdbId)
{
	Clear();

	CString sUrl;
	sUrl.Format(_T("http://api.themoviedb.org/2.1/Movie.getInfo/en/xml/%s/%d"), CAPI::apiKey, tmdbId);
	CString xmlFile = CNewzflow::Instance()->settings->GetAppDataDir() + _T("temp.xml");
	if(!downloader.Download(sUrl, xmlFile, CString(), NULL))
		return false;
	if(!Parse(xmlFile))
		return false;
	CFile::Delete(xmlFile);
	return true;
}

// CGetPerson
//////////////////////////////////////////////////////////////////////////

CGetPerson::CGetPerson()
{
	downloader.Init();
}

CGetPerson::~CGetPerson()
{
	Clear();
}

void CGetPerson::Clear()
{
	for(size_t i = 0; i < Persons.GetCount(); i++) delete Persons[i];
	Persons.RemoveAll();
}

bool CGetPerson::Parse(const CString& path)
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
			content->SetPersonArray(&Persons);
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

bool CGetPerson::Execute(int tmdbId)
{
	Clear();

	CString sUrl;
	sUrl.Format(_T("http://api.themoviedb.org/2.1/Person.getInfo/en/xml/%s/%d"), CAPI::apiKey, tmdbId);
	CString xmlFile = CNewzflow::Instance()->settings->GetAppDataDir() + _T("temp.xml");
	if(!downloader.Download(sUrl, xmlFile, CString(), NULL))
		return false;
	if(!Parse(xmlFile))
		return false;
	CFile::Delete(xmlFile);
	return true;
}

// CAPI
//////////////////////////////////////////////////////////////////////////

/*static*/ const CString CAPI::apiKey = _T("1a6c31ec26c8cf1d8c4c53814baa2451");

CAPI::CAPI()
{
}

CAPI::~CAPI()
{
}

CGetMovie* CAPI::GetMovie(const CString& imdbId)
{
	CGetMovie* getMovie = new CGetMovie;
	if(getMovie->Execute(imdbId))
		return getMovie;

	delete getMovie;
	return NULL;
}

CGetMovie* CAPI::GetMovie(int tmdbId)
{
	CGetMovie* getMovie = new CGetMovie;
	if(getMovie->Execute(tmdbId))
		return getMovie;

	delete getMovie;
	return NULL;
}

CGetPerson* CAPI::GetPerson(int tmdbId)
{
	CGetPerson* getPerson = new CGetPerson;
	if(getPerson->Execute(tmdbId))
		return getPerson;

	delete getPerson;
	return NULL;
}


} // namespace TheTvDB