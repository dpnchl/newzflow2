#include "stdafx.h"
#include <time.h>
#include "util.h"
#include "HttpDownloader.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

CHttpDownloader::CHttpDownloader()
{
}

CHttpDownloader::~CHttpDownloader()
{
}

bool CHttpDownloader::Init()
{
	hSession = WinHttpOpen(L"Newzflow/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,  WINHTTP_NO_PROXY_BYPASS, 0);
	return hSession != NULL;
}

bool CHttpDownloader::Download(const CString& url, const CString& dstFile)
{
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

	// Send a request.
	BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
	if(!bResults)
		return false;

	// End the request.
	bResults = WinHttpReceiveResponse(hRequest, NULL);
	if(!bResults)
		return false;

	CFile fout;
	if(!fout.Open(dstFile, GENERIC_WRITE, 0, CREATE_ALWAYS))
		return false;

	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	do {
		// Check for available data.
		dwSize = 0;
		if(!WinHttpQueryDataAvailable(hRequest, &dwSize))
			return false;

		// Allocate space for the buffer.
		char* pszOutBuffer = new char[dwSize];
		ZeroMemory(pszOutBuffer, dwSize);

		if(!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
			delete[] pszOutBuffer;
			return false;
		}

		fout.Write(pszOutBuffer, dwSize);

		// Free the memory allocated to the buffer.
		delete [] pszOutBuffer;
	} while(dwSize > 0);

	return true;
}
