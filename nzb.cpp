/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "nzb.h"
#include <msxml2.h>
#include <algorithm>
#pragma comment(lib, "msxml2.lib")


class MyContent : public CComObjectRootEx<CComSingleThreadModel>, public ISAXContentHandler
{
public:
	MyContent();
	virtual ~MyContent();

	CNzb* GetNzb() { return curNzb; }

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(MyContent)
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
	CNzb* curNzb;
	CNzbFile* curFile;
	CNzbSegment* curSegment;
	bool curGroup;
};

class SAXErrorHandlerImpl : public CComObjectRootEx<CComSingleThreadModel>, public ISAXErrorHandler
{
public:
	SAXErrorHandlerImpl();
	virtual ~SAXErrorHandlerImpl();

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(SAXErrorHandlerImpl)
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

MyContent::MyContent()
{
	curNzb = NULL;
	curFile = NULL;
	curSegment = NULL;
	curGroup = false;
}

MyContent::~MyContent()
{

}

HRESULT STDMETHODCALLTYPE MyContent::startElement( 
	/* [in] */ const wchar_t *pwchNamespaceUri,
	/* [in] */ int cchNamespaceUri,
	/* [in] */ const wchar_t *pwchLocalName,
	/* [in] */ int cchLocalName,
	/* [in] */ const wchar_t *pwchRawName,
	/* [in] */ int cchRawName,
	/* [in] */ ISAXAttributes *pAttributes)
{
	CString localName(pwchLocalName, cchLocalName);
	if(localName == _T("nzb")) {
		curNzb = new CNzb;
	} else if(localName == _T("file")) {
		ASSERT(curNzb);
		curFile = new CNzbFile(curNzb);
		const wchar_t* pwchSubject;
		int cchSubject;
		VERIFY(SUCCEEDED(pAttributes->getValueFromName(L"", 0, L"subject", wcslen(L"subject"), &pwchSubject, &cchSubject)));
		curFile->subject = CString(pwchSubject, cchSubject);
		curNzb->files.Add(curFile);
	} else if(localName == _T("group")) {
		ASSERT(curFile);
		curGroup = true;
	} else if(localName == _T("segment")) {
		ASSERT(curFile);
		curSegment = new CNzbSegment(curFile);
		const wchar_t* pwchBytes;
		int cchBytes;
		VERIFY(SUCCEEDED(pAttributes->getValueFromName(L"", 0, L"bytes", wcslen(L"bytes"), &pwchBytes, &cchBytes)));
		CString bytes(pwchBytes, cchBytes);
		curSegment->bytes = _wcstoui64(bytes, NULL, 10);

		const wchar_t* pwchNumber;
		int cchNumber;
		VERIFY(SUCCEEDED(pAttributes->getValueFromName(L"", 0, L"number", wcslen(L"number"), &pwchNumber, &cchNumber)));
		CString number(pwchNumber, cchNumber);
		curSegment->number = wcstoul(number, NULL, 10);

		curFile->segments.Add(curSegment);
	}
/*
	prt(L"<%s", pwchLocalName, cchLocalName);
	int lAttr;
	pAttributes->getLength(&lAttr);
	for(int i=0; i<lAttr; i++)
	{
		const wchar_t * ln;
		const wchar_t * vl;
		int lnl, vll;

		pAttributes->getLocalName(i,&ln,&lnl); 
		prt(L" %s=", ln, lnl);
		pAttributes->getValue(i,&vl,&vll);
		prt(L"\"%s\"", vl, vll);
	}
	printf(">"); 
*/
	return S_OK;
}

HRESULT STDMETHODCALLTYPE MyContent::characters( 
	/* [in] */ const wchar_t *pwchChars,
	/* [in] */ int cchChars)
{
	CString chars(pwchChars, cchChars);
	if(curGroup) {
		curFile->groups.Add(chars);
	} else if(curSegment) {
		curSegment->msgId = chars;
	}

//	prt(L"%s", pwchChars, cchChars); 
	return S_OK;
}

HRESULT STDMETHODCALLTYPE MyContent::endElement( 
	/* [in] */ const wchar_t *pwchNamespaceUri,
	/* [in] */ int cchNamespaceUri,
	/* [in] */ const wchar_t *pwchLocalName,
	/* [in] */ int cchLocalName,
	/* [in] */ const wchar_t *pwchRawName,
	/* [in] */ int cchRawName)
{
	CString localName(pwchLocalName, cchLocalName);
	if(localName == _T("group")) {
		curGroup = false;
	} else if(localName == _T("segment")) {
		curSegment = NULL;
	} else if(localName == _T("file")) {
		struct SortSegments {
			bool operator()(CNzbSegment* s1, CNzbSegment* s2) {
				return s1->number < s2->number;
			}
		};
		std::sort(curFile->segments.GetData(), curFile->segments.GetData() + curFile->segments.GetCount(), SortSegments());
		curFile = NULL;
	} else if(localName == _T("nzb")) {
		// caller wants to get final nzb with GetNzb() { return curNzb; }
	}

//	prt(L"</%s>",pwchLocalName,cchLocalName); 
	return S_OK;
}

HRESULT STDMETHODCALLTYPE MyContent::startDocument()
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE MyContent::putDocumentLocator( 
	/* [in] */ ISAXLocator *pLocator
	)
{
	return S_OK;
}



HRESULT STDMETHODCALLTYPE MyContent::endDocument( void)
{
	return S_OK;
}


HRESULT STDMETHODCALLTYPE MyContent::startPrefixMapping( 
	/* [in] */ const wchar_t *pwchPrefix,
	/* [in] */ int cchPrefix,
	/* [in] */ const wchar_t *pwchUri,
	/* [in] */ int cchUri)
{
	return S_OK;
}


HRESULT STDMETHODCALLTYPE MyContent::endPrefixMapping( 
	/* [in] */ const wchar_t *pwchPrefix,
	/* [in] */ int cchPrefix)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE MyContent::ignorableWhitespace( 
	/* [in] */ const wchar_t *pwchChars,
	/* [in] */ int cchChars)
{
	return S_OK;
}


HRESULT STDMETHODCALLTYPE MyContent::processingInstruction( 
	/* [in] */ const wchar_t *pwchTarget,
	/* [in] */ int cchTarget,
	/* [in] */ const wchar_t *pwchData,
	/* [in] */ int cchData)
{
	return S_OK;
}


HRESULT STDMETHODCALLTYPE MyContent::skippedEntity( 
	/* [in] */ const wchar_t *pwchVal,
	/* [in] */ int cchVal)
{
	return S_OK;
}

SAXErrorHandlerImpl::SAXErrorHandlerImpl()
{

}

SAXErrorHandlerImpl::~SAXErrorHandlerImpl()
{

}

HRESULT STDMETHODCALLTYPE SAXErrorHandlerImpl::error( 
	/* [in] */ ISAXLocator *pLocator,
	/* [in] */ const wchar_t * pwchErrorMessage,
	/* [in] */ HRESULT errCode)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SAXErrorHandlerImpl::fatalError( 
	/* [in] */ ISAXLocator *pLocator,
	/* [in] */ const wchar_t * pwchErrorMessage,
	/* [in] */ HRESULT errCode)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SAXErrorHandlerImpl::ignorableWarning( 
	/* [in] */ ISAXLocator *pLocator,
	/* [in] */ const wchar_t * pwchErrorMessage,
	/* [in] */ HRESULT errCode)
{
	return S_OK;
}

CString GetNzbStatusString(ENzbStatus status, float done /*= 0.f*/)
{
	switch(status) {
	case kQueued:		return _T("Queued");
	case kPaused:		return _T("Paused");
	case kDownloading:	return _T("Downloading");
	case kCompleted:	return _T("Completed");
	case kError:		return _T("Error");
	case kVerifying:	{ CString s; s.Format(_T("Verifying %.1f%%"), done); return s; }
	case kFinished:		return _T("Finished");
	default:			return _T("???");
	}
}

CString GetParStatusString(EParStatus status, float done)
{
	switch(status) {
	case kUnknown:		return _T("");
	case kScanning:		{ CString s; s.Format(_T("Scanning %.1f%%"), done); return s; }
	case kMissing:		return _T("Missing");
	case kFound:		return _T("Found");
	case kDamaged:		return _T("Damaged");
	default:			return _T("???");
	}
}

CNzb* CNzb::Create(const CString& path)
{
	CComPtr<ISAXXMLReader> pRdr;
	CoInitialize(NULL);
	HRESULT hr = pRdr.CoCreateInstance(__uuidof(SAXXMLReader30));
	ASSERT(!FAILED(hr));
	CComObject<MyContent>* content;
	CComObject<MyContent>::CreateInstance(&content);
	hr = pRdr->putContentHandler(content);
	ASSERT(!FAILED(hr));
	CComObject<SAXErrorHandlerImpl>* error;
	CComObject<SAXErrorHandlerImpl>::CreateInstance(&error);
	hr = pRdr->putErrorHandler(error);
	ASSERT(!FAILED(hr));
	hr = pRdr->parseURL(path);
	ASSERT(!FAILED(hr));
	CoUninitialize();
	content->GetNzb()->name = path; // TODO
	return content->GetNzb();
}

CNzbFile* CNzb::FindByName(const CString& name)
{
	for(size_t i = 0; i < files.GetCount(); i++) {
		if(name.CompareNoCase(files[i]->fileName) == 0)
			return files[i];
	}
	return NULL;
}
