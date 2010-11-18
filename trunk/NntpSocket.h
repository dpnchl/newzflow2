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
	int Get();

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

	void SetLastCommand(LPCTSTR fmt, ...);
	void SetLastReply(LPCTSTR fmt, ...);

	CString GetLastCommand();
	CString GetLastReply();

protected:
	CString GetLastError();

	SOCKET sock;

	CStringA user, passwd;

	CFifoBuffer recvBuffer;

	time_t timeStart, timeEnd;
	__int64 bytesReceived, bytesSent;

public:
	CSpeedMonitor speed;
	static CSpeedMonitor totalSpeed;

protected:
	CComAutoCriticalSection cs; // protects lastCommand and lastReply
	CString lastCommand, lastReply;

};