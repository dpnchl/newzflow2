#pragma once
#include "sock.h"

class CDownloader : public CThreadImpl<CDownloader>
{
public:
	CDownloader();

	DWORD Run();

	CNntpSocket sock;

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