#pragma once
#include "sock.h"

class CDownloader : public CThreadImpl<CDownloader>
{
public:
	DWORD Run();

	CNntpSocket sock;
};