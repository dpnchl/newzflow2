#include "stdafx.h"
#include <time.h>
#include "util.h"
#include "HttpDownloader.h"
#include "zlib/zlib.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// TODO: proxy support; see http://msdn.microsoft.com/en-us/magazine/cc716528.aspx

CHttpDownloader::CHttpDownloader()
{
}

CHttpDownloader::~CHttpDownloader()
{
}

bool CHttpDownloader::Init()
{
	hSession = WinHttpOpen(L"Newzflow/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,  WINHTTP_NO_PROXY_BYPASS, 0);
	if(!hSession)
		return false;

//	VERIFY(WINHTTP_INVALID_STATUS_CALLBACK != WinHttpSetStatusCallback(hSession, CHttpDownloader::StatusCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, NULL));
	return true;
}

// downloads a file via HTTP/HTTPS from url to dstFile
// filename as indicated by HTTP server is stored in outFilename
// progress in bytes will be written to *progress if progress != NULL
bool CHttpDownloader::Download(const CString& url, const CString& dstFile, CString& outFilename, float* progress)
{
	if(progress)
		*progress = 0.f;

	CAutoWinHttpHandle hConnect, hRequest;

	if(!hSession)
		return false;

	URL_COMPONENTS urlComp;
	ZeroMemory(&urlComp, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(urlComp);

	// Set required component lengths to non-zero so that they are cracked.
	urlComp.dwSchemeLength    = (DWORD)-1;
	urlComp.dwHostNameLength  = (DWORD)-1;
	urlComp.dwUrlPathLength   = (DWORD)-1;
	urlComp.dwExtraInfoLength = (DWORD)-1;
	urlComp.dwUserNameLength  = (DWORD)-1;
	urlComp.dwPasswordLength  = (DWORD)-1;

	if(!WinHttpCrackUrl(url, url.GetLength(), 0, &urlComp))
		return false;

	if(!urlComp.lpszHostName)
		return false;

	// Specify an HTTP server.
	hConnect = WinHttpConnect(hSession, CString(urlComp.lpszHostName, urlComp.dwHostNameLength), urlComp.nPort, 0);
	if(!hConnect)
		return false;

	// Create an HTTP request handle.
	hRequest = WinHttpOpenRequest(hConnect, L"GET", urlComp.lpszUrlPath ? urlComp.lpszUrlPath : NULL, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, urlComp.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
	if(!hRequest)
		return false;

	if(!WinHttpAddRequestHeaders(hRequest, L"Accept-Encoding: gzip", -1L, WINHTTP_ADDREQ_FLAG_ADD))
		return false;

	// Set username/password if specified in URL
	if(urlComp.lpszUserName && urlComp.lpszPassword) {
		if(!WinHttpSetCredentials(hRequest, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_BASIC, CString(urlComp.lpszUserName, urlComp.dwUserNameLength), CString(urlComp.lpszPassword, urlComp.dwPasswordLength), NULL))
			return false;
	}

	// Send a request.
	if(!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
		return false;

	// End the request.
	if(!WinHttpReceiveResponse(hRequest, NULL))
		return false;

    // dump headers for debugging purposes
	if(0) {
		DWORD dwSize = 0;
		WinHttpQueryHeaders( hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &dwSize, WINHTTP_NO_HEADER_INDEX);

		// Allocate memory for the buffer.
		WCHAR* lpOutBuffer = new WCHAR[dwSize/sizeof(WCHAR)];

		// Now, use WinHttpQueryHeaders to retrieve the header.
		WinHttpQueryHeaders( hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, lpOutBuffer, &dwSize, WINHTTP_NO_HEADER_INDEX);

		Util::CreateConsole();
		wprintf(L"Headers: %s\n", lpOutBuffer);
		delete[] lpOutBuffer;

		/*
		NZBMATRIX:
			HTTP/1.1 200 OK
			Date: Sat, 20 Nov 2010 10:29:02 GMT
			Server: Apache/2.0.63 (Unix) mod_ssl/2.0.63 OpenSSL/0.9.8e-fips-rhel5 mod_bwlimited/1.4
			X-Powered-By: PHP/5.2.9
			API_Rate_Limit: 100
			API_Rate_Limit_Used: 2
			API_Rate_Limit_Left: 98
			API_Download_Limit: 150
			API_Download_Limit_Used: 3
			API_Download_Limit_Left: 147
			Category_ID: 20
			Content-Disposition: attachment; filename="BlaBlaFileName.nzb"
			Keep-Alive: timeout=15, max=100
			Connection: Keep-Alive
			Transfer-Encoding: chunked
			Content-Type: application/x-nzb

		NZBINDEX:
			HTTP/1.1 200 OK
			Server: nginx
			Date: Sat, 20 Nov 2010 10:31:18 GMT
			Content-Type: application/x-nzb
			Transfer-Encoding: chunked
			Connection: close
			Vary: Accept-Encoding
			X-Powered-By: PHP/5.2.6-3ubuntu4.4
			Set-Cookie: PHPSESSID=ba6be89df29cf655987c001df905f1fe; path=/
			Set-Cookie: SPAW_DIR=%2Fadmin%2Fspaw2%2F; expires=Sun, 20-Nov-2011 10:31:18 GMT; path=/
			Set-Cookie: lang=1; expires=Sun, 20-Nov-2011 10:31:18 GMT; path=/
			Content-Disposition: attachment; filename="ubuntu-10.10-server-i386.nzb"
			Pragma: cache
			Cache-Control: max-age=86400
			Expires: Sun, 21 Nov 2010 10:31:18 GMT
			Last-Modified: Sat, 20 Nov 2010 11:31:18 +0100

		BINSEARCH:
			HTTP/1.1 200 OK
			X-Powered-By: PHP/5.2.14
			P3P: CP="NON DSP COR CURa ADMa DEVa TAIa OUR SAMa IND"
			Content-Type: application/x-nzb
			Content-Disposition: attachment; filename="45248169.nzb"
			Robots: NOINDEX
			Transfer-Encoding: chunked
			Date: Sat, 20 Nov 2010 10:40:40 GMT
			Server: lighttpd/1.4.26
		*/
	}

	outFilename = GetFilename(hRequest);

	DWORD statusCode = 0;
	DWORD statusCodeSize = sizeof(DWORD);
	if(!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX))
		return false;

	if(statusCode != 200)
		return false;

	CStringW contentEncoding;
	DWORD contentEncodingSize = 0;
	WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_ENCODING, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &contentEncodingSize, NULL);
	WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_ENCODING, WINHTTP_HEADER_NAME_BY_INDEX, contentEncoding.GetBuffer(contentEncodingSize/sizeof(WCHAR)), &contentEncodingSize, NULL);
	contentEncoding.ReleaseBuffer();

	CFile fout;
	if(!fout.Open(dstFile, GENERIC_WRITE, 0, CREATE_ALWAYS))
		return false;

	bool gzip = false;
	if(contentEncoding == "gzip") gzip = true;

	z_stream zstream;
	memset(&zstream, 0, sizeof(zstream));
	char* decompBuffer = new char[64*1024];
	if(gzip) {
		if(Z_OK != inflateInit2(&zstream, 15 + 32)) { // enough windowbits to detect gzip header (see http://www.deez.info/sengelha/2004/09/01/in-memory-decompression-of-gzip-using-zlib/)
			delete[] decompBuffer;
			return false;
		}
	}

	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	do {
		// Check for available data.
		dwSize = 0;
		if(!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
			delete[] decompBuffer;
			fout.Close();
			DeleteFile(dstFile);
			if(gzip) inflateEnd(&zstream);
			return false;
		}

		if(dwSize == 0)
			break;

		// Allocate space for the buffer.
		char* pszOutBuffer = new char[dwSize];
		ZeroMemory(pszOutBuffer, dwSize);

		if(!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
			delete[] decompBuffer;
			delete[] pszOutBuffer;
			fout.Close();
			DeleteFile(dstFile);
			if(gzip) inflateEnd(&zstream);
			return false;
		}

		if(gzip) {
			zstream.next_in = (Bytef*)pszOutBuffer;
			zstream.avail_in = dwSize;
			zstream.next_out = (Bytef*)decompBuffer;
			zstream.avail_out = 64*1024;
			do {
				int zres = inflate(&zstream, Z_SYNC_FLUSH);
				if(zres < 0) {
					delete[] decompBuffer;
					delete[] pszOutBuffer;
					fout.Close();
					DeleteFile(dstFile);
					inflateEnd(&zstream);
					return false;
				}
				// decompBuffer overflow
				if(zstream.avail_out == 0) {
					if(progress)
						*progress += (float)dwSize;
					fout.Write(decompBuffer, (char*)zstream.next_out - decompBuffer);
					zstream.next_out = (Bytef*)decompBuffer;
					zstream.avail_out = 64*1024;
				}
			} while(zstream.avail_in > 0);

			if(progress)
				*progress += (float)dwSize;

			fout.Write(decompBuffer, (char*)zstream.next_out - decompBuffer);
		} else {
			if(progress)
				*progress += (float)dwSize;

			fout.Write(pszOutBuffer, dwSize);
		}

		// Free the memory allocated to the buffer.
		delete [] pszOutBuffer;
	} while(dwSize > 0);

	if(gzip) inflateEnd(&zstream);
	delete[] decompBuffer;

	return true;
}

// currently not used
void CALLBACK CHttpDownloader::StatusCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
{
	_tprintf(_T("%08x %08x %08x"), hInternet, dwContext, dwInternetStatus);
	switch(dwInternetStatus) {
	case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
		_tprintf(_T("WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE: %d\n"), *(DWORD*)lpvStatusInformation);
		break;
	case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
		_tprintf(_T("WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE\n"));
		break;
	case WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE:
		_tprintf(_T("WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE: %d\n"), *(DWORD*)lpvStatusInformation);
		break;
	case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
		_tprintf(_T("WINHTTP_CALLBACK_STATUS_READ_COMPLETE: %d\n"), dwStatusInformationLength);
		break;
	case WINHTTP_CALLBACK_STATUS_REDIRECT:
		_tprintf(_T("WINHTTP_CALLBACK_STATUS_REDIRECT: %s\n"), (LPWSTR*)lpvStatusInformation);
		break;
	case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
		_tprintf(_T("WINHTTP_CALLBACK_STATUS_REQUEST_ERROR: %d\n"), ((WINHTTP_ASYNC_RESULT*)lpvStatusInformation)->dwResult);
		break;
	case WINHTTP_CALLBACK_STATUS_SECURE_FAILURE:
		_tprintf(_T("WINHTTP_CALLBACK_STATUS_SECURE_FAILURE: "));
		switch((DWORD)lpvStatusInformation) {
		case WINHTTP_CALLBACK_STATUS_FLAG_CERT_REV_FAILED:
			_tprintf(_T("WINHTTP_CALLBACK_STATUS_FLAG_CERT_REV_FAILED\n"));
			break;
		case WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CERT:
			_tprintf(_T("WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CERT\n"));
			break;
		case WINHTTP_CALLBACK_STATUS_FLAG_CERT_REVOKED:
			_tprintf(_T("WINHTTP_CALLBACK_STATUS_FLAG_CERT_REVOKED\n"));
			break;
		case WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CA:
			_tprintf(_T("WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CA\n"));
			break;
		case WINHTTP_CALLBACK_STATUS_FLAG_CERT_CN_INVALID:
			_tprintf(_T("WINHTTP_CALLBACK_STATUS_FLAG_CERT_CN_INVALID\n"));
			break;
		case WINHTTP_CALLBACK_STATUS_FLAG_CERT_DATE_INVALID:
			_tprintf(_T("WINHTTP_CALLBACK_STATUS_FLAG_CERT_DATE_INVALID\n"));
			break;
		case WINHTTP_CALLBACK_STATUS_FLAG_SECURITY_CHANNEL_ERROR:
			_tprintf(_T("WINHTTP_CALLBACK_STATUS_FLAG_SECURITY_CHANNEL_ERROR\n"));
			break;
		}
		break;
	case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
		_tprintf(_T("WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE\n"));
		break;
	case WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE:
		_tprintf(_T("WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE: %d\n"), dwStatusInformationLength);
		break;
	default:
		_tprintf(_T("\n"));
		break;
	}
}

CString CHttpDownloader::GetLastError()
{
	int errCode = ::GetLastError();

	LPTSTR errString = NULL;  // will be allocated and filled by FormatMessage
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, errCode, 0, (LPTSTR)&errString, 0, 0);

	CString s(errString);
	LocalFree(errString);

	return s;
}

CString CHttpDownloader::GetHeader(HINTERNET hRequest, DWORD infoLevel)
{
	DWORD dwSize = 0;
	CString s;
	WinHttpQueryHeaders(hRequest, infoLevel, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &dwSize, WINHTTP_NO_HEADER_INDEX);
	WinHttpQueryHeaders(hRequest, infoLevel, WINHTTP_HEADER_NAME_BY_INDEX, s.GetBufferSetLength(dwSize), &dwSize, WINHTTP_NO_HEADER_INDEX);
	s.ReleaseBuffer();
	return s;
}

CString CHttpDownloader::GetOption(HINTERNET hRequest, DWORD option)
{
	DWORD dwSize = 0;
	CString s;
	WinHttpQueryOption(hRequest, option, NULL, &dwSize);
	WinHttpQueryOption(hRequest, option, s.GetBufferSetLength(dwSize), &dwSize);
	s.ReleaseBuffer();
	return s;
}

// Get the filename for a WinHTTP Request
// Returns an empty CString if filename can't be determined
CString CHttpDownloader::GetFilename(HINTERNET hRequest)
{
	// first try to extract the filename from the Content-Disposition HTTP header
	CString contentDisposition = GetHeader(hRequest, WINHTTP_QUERY_CONTENT_DISPOSITION);
	CString sLower(contentDisposition); sLower.MakeLower();
	int start = sLower.Find(_T("filename=\""));
	int end = sLower.ReverseFind('"');
	if(start >= 0 && end > start + 9) {
		CString fileName = Util::SanitizeFilename(contentDisposition.Mid(start + 10, end - (start + 10)));
		return fileName;
	} else {
		// if that failed, get it from the URL; Get the URL here again because it may have changed due to redirect(s)
		CString url = GetOption(hRequest, WINHTTP_OPTION_URL);

		URL_COMPONENTS urlComp;
		ZeroMemory(&urlComp, sizeof(urlComp));
		urlComp.dwStructSize = sizeof(urlComp);

		// Set required component lengths to non-zero so that they are cracked.
		urlComp.dwUrlPathLength   = (DWORD)-1;
		urlComp.dwExtraInfoLength = (DWORD)-1;

		if(WinHttpCrackUrl(url, url.GetLength(), 0, &urlComp) && urlComp.lpszUrlPath) {
			CString path(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
			int slash = path.ReverseFind('/');
			if(slash >= 0)
				path = path.Mid(slash + 1);
			path = Util::SanitizeFilename(path);
			return path;
		}
	}
	return _T("");
}
