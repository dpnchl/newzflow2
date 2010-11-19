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

	bool Download(const CString& url, const CString& dstFile);

protected:
	CAutoWinHttpHandle hSession;
};