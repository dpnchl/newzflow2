#pragma once

class CAutoWinHttpHandle
{
public:
	CAutoWinHttpHandle() {
		handle = NULL;
	}
	~CAutoWinHttpHandle() {
		if(handle) WinHttpCloseHandle(handle);
	}

	operator HINTERNET() { return handle; }
	CAutoWinHttpHandle& operator =(HINTERNET _handle) { handle = _handle; return *this; }

protected:
	HINTERNET handle;
};

class CHttpDownloader
{
public:
	CHttpDownloader();
	~CHttpDownloader();

	bool Init();
	bool Download(const CString& url, const CString& dstFile, CString& outFilename, float* progress);

	CString GetFilename(HINTERNET hRequest);
	CString GetLastError();

protected:
	CString GetHeader(HINTERNET hRequest, DWORD infoLevel);
	CString GetOption(HINTERNET hRequest, DWORD option);
	static void CALLBACK StatusCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);

	CAutoWinHttpHandle hSession;
};