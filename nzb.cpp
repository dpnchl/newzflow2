/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "nzb.h"
#include "Newzflow.h"
#include "Settings.h"
#include "Util.h"

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

namespace {

class CNzbParser : public CComObjectRootEx<CComSingleThreadModel>, public ISAXContentHandler
{
public:
	CNzbParser();
	virtual ~CNzbParser();

	void SetNzb(CNzb* nzb) { theNzb = nzb; }

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CNzbParser)
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
	CNzb* theNzb;
	CNzb* curNzb;
	CNzbFile* curFile;
	CNzbSegment* curSegment;
	bool curGroup;
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

CNzbParser::CNzbParser()
{
	curNzb = NULL;
	curFile = NULL;
	curSegment = NULL;
	curGroup = false;
}

CNzbParser::~CNzbParser()
{
}

HRESULT STDMETHODCALLTYPE CNzbParser::startElement( 
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
		ASSERT(theNzb);
		curNzb = theNzb;
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
	return S_OK;
}

// TODO: handle split calls due to entities (see CRssParser::characters())
HRESULT STDMETHODCALLTYPE CNzbParser::characters( 
	/* [in] */ const wchar_t *pwchChars,
	/* [in] */ int cchChars)
{
	CString chars(pwchChars, cchChars);
	if(curGroup) {
		curFile->groups.Add(chars);
	} else if(curSegment) {
		curSegment->msgId = chars;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CNzbParser::endElement( 
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
		curFile = NULL;
	} else if(localName == _T("nzb")) {
		curNzb = NULL;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CNzbParser::startDocument()
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CNzbParser::putDocumentLocator( 
	/* [in] */ ISAXLocator *pLocator
	)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CNzbParser::endDocument( void)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CNzbParser::startPrefixMapping( 
	/* [in] */ const wchar_t *pwchPrefix,
	/* [in] */ int cchPrefix,
	/* [in] */ const wchar_t *pwchUri,
	/* [in] */ int cchUri)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CNzbParser::endPrefixMapping( 
	/* [in] */ const wchar_t *pwchPrefix,
	/* [in] */ int cchPrefix)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CNzbParser::ignorableWhitespace( 
	/* [in] */ const wchar_t *pwchChars,
	/* [in] */ int cchChars)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CNzbParser::processingInstruction( 
	/* [in] */ const wchar_t *pwchTarget,
	/* [in] */ int cchTarget,
	/* [in] */ const wchar_t *pwchData,
	/* [in] */ int cchData)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CNzbParser::skippedEntity( 
	/* [in] */ const wchar_t *pwchVal,
	/* [in] */ int cchVal)
{
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

CString GetNzbStatusString(ENzbStatus status, float done /*= 0.f*/, int setDone /*= 0*/, int setTotal /*= 0*/)
{
	CString t;
	if(setTotal > 1) 
		t.Format(_T("(%d/%d) "), setDone, setTotal);

	switch(status) {
	case kEmpty:		return _T("Initializing");
	case kFetching:		{ CString s; s.Format(_T("Fetching %s"), Util::FormatSize((__int64)done)); return s; }
	case kQueued:		return _T("Queued");
	case kPaused:		return _T("Paused");
	case kDownloading:	return _T("Downloading");
	case kCached:		return _T("Cached");
	case kCompleted:	return _T("Completed");
	case kError:		return _T("Error");
	case kVerifying:	{ CString s; s.Format(_T("Verifying %s%.1f%%"), t, done); return s; }
	case kRepairing:	{ CString s; s.Format(_T("Repairing %s%.1f%%"), t, done); return s; }
	case kUnpacking:	{ CString s; s.Format(_T("Unpacking %s%.1f%%"), t, done); return s; }
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

bool CNzb::Parse(const CString& path)
{
	bool retCode = false;

	// parse NZB file
	{
		CComPtr<ISAXXMLReader> pRdr;
		CoInitialize(NULL);
		HRESULT hr = pRdr.CoCreateInstance(__uuidof(SAXXMLReader30));
		if(SUCCEEDED(hr)) {
			CComObject<CNzbParser>* content;
			CComObject<CNzbParser>::CreateInstance(&content);
			content->SetNzb(this);
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
		Init();
	}

	return retCode;
}

// add new NZB to queue
// NZB is on disk and will be copied to %appdata%\Newzflow
bool CNzb::CreateFromPath(const CString& nzbPath)
{
	if(!Parse(nzbPath))
		return false;

	status = kQueued;

	// pause all PARs except for the smallest in each ParSet
	for(size_t i = 0; i < parSets.GetCount(); i++) {
		CParSet* parSet = parSets[i];
		// PARs are sorted by size, that's why we start at 1, and not at 0
		for(size_t j = 1; j < parSet->pars.GetCount(); j++)
			parSet->pars[j]->file->status = kPaused;
	}

	// set NZB name
	int slash = nzbPath.ReverseFind('\\');
	if(slash >= 0)
		name = nzbPath.Mid(slash+1);
	else
		name = nzbPath;

	CopyFile(nzbPath, GetLocalPath(), FALSE);

	return true;
}

// add new NZB to queue
// NZB has been downloaded from HTTP and is already copied to %appdata%\Newzflow
bool CNzb::CreateFromLocal()
{
	if(!Parse(GetLocalPath()))
		return false;

	status = kQueued;

	// pause all PARs except for the smallest in each ParSet
	for(size_t i = 0; i < parSets.GetCount(); i++) {
		CParSet* parSet = parSets[i];
		// PARs are sorted by size, that's why we start at 1, and not at 0
		for(size_t j = 1; j < parSet->pars.GetCount(); j++)
			parSet->pars[j]->file->status = kPaused;
	}

	return true;
}

// restore NZB from queue
// NZB is already copied to %appdata%\Newzflow
bool CNzb::CreateFromQueue(REFGUID _guid, const CString& _name)
{
	name = _name;
	guid = _guid; // restore GUID from queue

	if(!Parse(GetLocalPath()))
		return false;

	status = kQueued;

	return true;
}

CString CNzb::GetLocalPath()
{
	return CNewzflow::Instance()->settings->GetAppDataDir() + CComBSTR(guid) + _T(".nzb");
}

void CNzb::SetPath(LPCTSTR _path, LPCTSTR _name)
{
	ASSERT(_path);
	path = _path;
	if(path.Right(1) != _T("\\")) path += '\\';
	if(_name) 
		path += _name;
	else
		path += name;
}

CNzbFile* CNzb::FindFile(const CString& name)
{
	for(size_t i = 0; i < files.GetCount(); i++) {
		if(name.CompareNoCase(files[i]->fileName) == 0)
			return files[i];
	}
	return NULL;
}

void CNzb::Init()
{
	for(size_t i = 0; i < files.GetCount(); i++) {
		CNzbFile* file = files[i];
		file->Init(); // parse subject into filename, init segments' offsets and filesize
	}

	// get ParSets
	using std::tr1::tregex; using std::tr1::tcmatch; using std::tr1::regex_search; using std::tr1::regex_match;
	tregex rePar2(_T("(.*?)(\\.vol\\d+\\+(\\d+))?\\.par2"));
	tcmatch match;

	for(size_t i = 0; i < files.GetCount(); i++) {
		CNzbFile* file = files[i];
		CString fn = files[i]->fileName;
		fn.MakeLower();
		// check if file is a par2 file
		if(regex_match((const TCHAR*)fn, match, rePar2)) {
			CString parName(match[1].first, match[1].length());
			// find corresponding CParSet
			CParSet* parSet = NULL;
			for(size_t j = 0; j < parSets.GetCount(); j++) {
				if(parSets[j]->baseName == parName) {
					parSet = parSets[j];
				}
			}
			// create new ParSet if not found
			if(!parSet) {
				parSet = new CParSet(this);
				parSet->baseName = parName;
				parSets.Add(parSet);
			}
			CParFile* parFile = new CParFile(parSet);
			parFile->file = file;
			parFile->numBlocks = 0;
			if(match[3].matched) {
				CString blocks(match[3].first, match[3].length());
				parFile->numBlocks = _ttoi(blocks);
			}
			// add par2 file to ParSet
			parSet->pars.Add(parFile);
		}
	}
	// now sort par2 files in par sets by their number of blocks
	// this way we can pause all par files except the first one (the one with the lowest number of blocks)
	struct SortPars {
		bool operator()(CParFile* s1, CParFile* s2) {
			return s1->numBlocks < s2->numBlocks;
		}
	};
	for(size_t i = 0; i < parSets.GetCount(); i++) {
		CParSet* parSet = parSets[i];
		std::sort(parSet->pars.GetData(), parSet->pars.GetData() + parSet->pars.GetCount(), SortPars());
	}
	// don't pause par files here, thie method might also be called when queue is restored, and we don't want to pause files then
}

void CNzb::Cleanup()
{
	DeleteFile(GetLocalPath());
	Util::DeleteDirectory(CNewzflow::Instance()->settings->GetAppDataDir() + CComBSTR(guid));
}

// CNzbFile
//////////////////////////////////////////////////////////////////////////

void CNzbFile::Init()
{
	// parse subject for filename
	int firstQuote = subject.Find('\"');
	int lastQuote = subject.ReverseFind('\"');
	if(firstQuote != -1 && lastQuote != -1 && lastQuote > firstQuote)
		fileName = Util::SanitizeFilename(subject.Mid(firstQuote+1, lastQuote - firstQuote - 1));
	else
		fileName = Util::SanitizeFilename(subject);

	struct SortSegments {
		bool operator()(CNzbSegment* s1, CNzbSegment* s2) {
			return s1->number < s2->number;
		}
	};

	// sort segments by number
	std::sort(segments.GetData(), segments.GetData() + segments.GetCount(), SortSegments());

	// calc filesize (encoded file)
	size = 0;
	for(size_t j = 0; j < segments.GetCount(); j++) {
		CNzbSegment* seg = segments[j];
		size += seg->bytes;
	}
}
