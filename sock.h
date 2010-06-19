#pragma once

class CFifoBuffer
{
public:
	CFifoBuffer();
	CFifoBuffer(int size);
	~CFifoBuffer();

	void Create(int size);
	void Clear();
	void Replace(const void* ptr, int len); // memmove is used, so it's okay if ptr lies in buffer

	void Fill(const void* ptr, int len); // append "len" bytes from "ptr"; call with ptr=NULL after manual fill with GetFillPtr/GetFillSize
	char* GetFillPtr(); // get pointer where to append
	int GetFillSize(); // get max. length to append

	int Consume(void* dst, int len);

	char* GetBuffer();
	int GetSize();

	bool IsEmpty();

protected:
	void Realloc(int newsize);

	char* buffer;
	int size, maxSize;
};

class CLineBuffer : public CFifoBuffer
{
public:
	CLineBuffer();
	CLineBuffer(int size);

	CString GetLine();
};

class CSpeedMonitor
{
public:
	CSpeedMonitor(int _slots);
	~CSpeedMonitor();

	void OnReceive(int bytes);
	float Get();

protected:
	void Trim();

	CComAutoCriticalSection cs;

	int slots;
	CAtlList<std::pair<time_t, int> > history;
};

class CNntpSocket
{
public:
	CNntpSocket();
	~CNntpSocket();

	static bool InitWinsock();
	static void CloseWinsock();

	bool Connect(const CString& host, const CString& service, const CString& user, const CString& passwd);
	void Close();

	CStringA Request(const char* fmt, ...);

	bool Send(const char* fmt, ...);
	bool SendV(const char* fmt, va_list va);
	CStringA ReceiveLine();


protected:
	SOCKET s;

	CStringA user, passwd;

	CFifoBuffer recvBuffer;

	time_t timeStart, timeEnd;
	__int64 bytesReceived, bytesSent;

public:
	CComAutoCriticalSection cs;
	CSpeedMonitor speed;
	CStringA lastCommand;

	static CSpeedMonitor totalSpeed;
};