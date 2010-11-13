#include "stdafx.h"
#include <time.h>
#include "util.h"
#include "sock.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// CFifoBuffer
//////////////////////////////////////////////////////////////////////////

CFifoBuffer::CFifoBuffer()
{
	buffer = NULL;
	size = maxSize = 0;
}

CFifoBuffer::CFifoBuffer(int _size)
{
	buffer = NULL;
	size = maxSize = 0;
	Realloc(_size);
}

CFifoBuffer::~CFifoBuffer()
{
	delete buffer;
}

void CFifoBuffer::Create(int _size)
{
	size = 0;
	Realloc(_size);
}

void CFifoBuffer::Fill(const void* ptr, int len)
{
	if(ptr) {
		Realloc(size + len);
		memcpy(buffer + size, ptr, len);
	}
	size += len;
}

char* CFifoBuffer::GetFillPtr()
{
	return buffer + size;
}

int CFifoBuffer::GetFillSize()
{
	return maxSize - size;
}

int CFifoBuffer::Consume(void* dst, int len)
{
	if(len > size) len = size;
	if(dst) {
		memcpy(dst, buffer, len);
	}
	size -= len;
	memmove(buffer, buffer + len, size);
	return len;
}

char* CFifoBuffer::GetBuffer()
{
	return buffer;
}

void CFifoBuffer::Realloc(int newsize)
{
	if(newsize <= maxSize)
		return;

	char* newBuffer = new char [newsize];
	if(buffer) {
		memcpy(newBuffer, buffer, size);
		delete buffer;
	}
	buffer = newBuffer;
	maxSize = newsize;
}

bool CFifoBuffer::IsEmpty()
{
	return size == 0;
}

int CFifoBuffer::GetSize()
{
	return size;
}

void CFifoBuffer::Clear()
{
	size = 0;
}

void CFifoBuffer::Replace(const void* ptr, int len)
{
	memmove(buffer, ptr, len);
	size = len;
}

// CLineBuffer
//////////////////////////////////////////////////////////////////////////

CLineBuffer::CLineBuffer()
: CFifoBuffer()
{
}

CLineBuffer::CLineBuffer(int size)
: CFifoBuffer(size)
{
}

CString CLineBuffer::GetLine()
{
	int crlf = -1;
	for(int i = 0; i < size; i++) {
		if(buffer[i] == '\r' || buffer[i] == '\n') {
			crlf = i;
			break;
		}
	}
	if(crlf == -1)
		return _T("");

	CString ret(CStringA(buffer, crlf));
	if(ret.IsEmpty()) ret = _T(" ");

	if(buffer[crlf] == '\r' && crlf < size-1 && buffer[crlf+1] == '\n')
		crlf++;

	Consume(NULL, crlf+1);

	return ret;
}

// CNntpSocket
//////////////////////////////////////////////////////////////////////////

CNntpSocket::CNntpSocket()
: speed(3)
{
	recvBuffer.Create(10*1024);
	s = INVALID_SOCKET;

	timeStart = timeEnd = 0;
	bytesReceived = bytesSent = 0;
}

CNntpSocket::~CNntpSocket()
{
	if(s != INVALID_SOCKET)
		Close();
}

bool CNntpSocket::InitWinsock()
{
	// initialize winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	return iResult == 0;
}

void CNntpSocket::CloseWinsock()
{
	WSACleanup();
}

bool CNntpSocket::Connect(const CString& host, const CString& service, const CString& _user, const CString& _passwd)
{
	// create socket
	ADDRINFOT *result = NULL, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	int iResult = GetAddrInfo(host, service, &hints, &result);
	ASSERT(iResult == 0);
	if (iResult != 0) {
		return false;
	}

	// try to connect to all returned addresses
	for(ADDRINFOT* ptr = result; ptr; ptr = ptr->ai_next) {
		// Create a SOCKET for connecting to server
		s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		ASSERT(s != INVALID_SOCKET);
		if(s == INVALID_SOCKET) {
			FreeAddrInfo(result);
			return false;
		}
		iResult = connect(s, ptr->ai_addr, (int)ptr->ai_addrlen);
		if(iResult == SOCKET_ERROR) {
			// connect failed, try next address
			closesocket(s);
			s = INVALID_SOCKET;
		} else {
			// connect succeeded
			break;
		}
	}
	FreeAddrInfo(result);
	timeStart = time(NULL);
	user = CStringA(_user);
	passwd = CStringA(_passwd);
	return s != INVALID_SOCKET;
}

CStringA CNntpSocket::ReceiveLine()
{
	while(1) {
		char* lf = (char*)memchr(recvBuffer.GetBuffer(), '\n', recvBuffer.GetSize());
		if(lf) {
			int lineLen = lf - recvBuffer.GetBuffer() + 1;
			CStringA line(recvBuffer.GetBuffer(), lineLen);
			recvBuffer.Consume(NULL, lineLen);
			return line;
		}
		int iResult = recv(s, recvBuffer.GetFillPtr(), recvBuffer.GetFillSize(), 0);
		if(iResult > 0) {
			recvBuffer.Fill(NULL, iResult);
			speed.OnReceive(iResult);
			CNntpSocket::totalSpeed.OnReceive(iResult);
			bytesReceived += iResult;
		} else
			return CStringA(); // error, return empty string
	}
}

bool CNntpSocket::Send(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	bool ret = SendV(fmt, args);
	va_end(args);
	return ret;
}

bool CNntpSocket::SendV(const char* fmt, va_list args)
{
	CStringA line;
	line.FormatV(fmt, args);
	va_end(args);
	Util::Print(line);
	{ CComCritSecLock<CComAutoCriticalSection> lock(cs);
		lastCommand = line;
	}
	int iResult = send(s, line, line.GetLength(), 0);
	bytesSent += iResult;
	return iResult != SOCKET_ERROR;
}

CStringA CNntpSocket::Request(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
again:
	bool ret = SendV(fmt, args);
	if(!ret)
		return "";
	CStringA reply = ReceiveLine(); Util::Print(reply);
	if(reply.IsEmpty())
		return "";
	if(reply.Left(3) == "480") {
		if(user.IsEmpty() || passwd.IsEmpty())
			return "";
		Send("AUTHINFO USER %s\n", user); 
		reply = ReceiveLine(); Util::Print(reply);
		if(reply.Left(3) == "281")
			goto again;
		if(reply.Left(3) == "381") {
			Send("AUTHINFO PASS %s\n", passwd);
			reply = ReceiveLine(); Util::Print(reply);
			if(reply.Left(3) == "281")
				goto again;
			return "";
		} else
			return "";
	}
	return reply;
}

void CNntpSocket::Close()
{
	timeEnd = time(NULL);
	CString str;
	str.Format(_T("rate: %s (%s sent, %s received in %s)"), Util::FormatSpeed(__int64(bytesReceived + bytesSent) / __int64(timeEnd - timeStart)), Util::FormatSize(bytesSent), Util::FormatSize(bytesReceived), Util::FormatTimeSpan(timeEnd - timeStart));
	Util::Print(CStringA(str));

	shutdown(s, SD_SEND);
	closesocket(s);
	s = INVALID_SOCKET;
}

/*static*/ CSpeedMonitor CNntpSocket::totalSpeed(3);

// CSpeedMonitor
//////////////////////////////////////////////////////////////////////////

CSpeedMonitor::CSpeedMonitor(int _slots)
{
	slots = _slots;
}

CSpeedMonitor::~CSpeedMonitor()
{

}

void CSpeedMonitor::OnReceive(int bytes)
{
	CComCritSecLock<CComAutoCriticalSection> lock(cs);

	time_t now = time(NULL);
	if(!history.IsEmpty() && history.GetHead().first == now)
		history.GetHead().second += bytes;
	else
		history.AddHead(std::make_pair(now, bytes));

	Trim();
}

int CSpeedMonitor::Get()
{
	CComCritSecLock<CComAutoCriticalSection> lock(cs);

	Trim();

	if(history.GetCount() < 2)
		return 0;

	// don't take head into account, it may still be active
	int bytes = 0;
	POSITION p = history.GetHeadPosition(); history.GetNext(p);
	time_t firstTime = history.GetAt(p).first;
	while(p) {
		bytes += history.GetNext(p).second;
	}

	time_t elapsed = firstTime - history.GetTail().first + 1;

	return (int)(bytes / elapsed);
}

// calling function locks!
void CSpeedMonitor::Trim()
{
	if(history.IsEmpty())
		return;

	time_t now = time(NULL);
	while(!history.IsEmpty() && history.GetTail().first < now - slots)
		history.RemoveTail();
}

