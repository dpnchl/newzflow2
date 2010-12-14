#pragma once

#include "NntpSocket.h"

class CDownloader : public CThreadImpl<CDownloader>
{
public:
	CDownloader();

	DWORD Run();

	CNntpSocket sock;
	CEvent shutDown; // CNewzflow will set this event to indicate this downloader should shut down

protected:
	enum EStatus {
		kSuccess,
		kTemporaryError,
		kPermanentError,
	};

	EStatus Connect();
	void Disconnect();
	EStatus DownloadSegment(CNzbSegment* segment);


	bool connected;
	CString curGroup;
};