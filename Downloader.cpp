#include "stdafx.h"
#include "util.h"
#include "DownloadQueue.h"
#include "Downloader.h"
#include "DiskWriter.h"
#include "sock.h"
#include "nzb.h"
#include "Newzflow.h"

// CYDecoder
//////////////////////////////////////////////////////////////////////////

class CYDecoder
{
public:
	CYDecoder();
	~CYDecoder();

	void ProcessLine(const char* line);

	CString fileName;
	__int64 offset;
	unsigned int size;
	char* buffer;

	char* ptr;
};

CYDecoder::CYDecoder()
{
	offset = size = 0;
	buffer = ptr = NULL;
}

CYDecoder::~CYDecoder()
{
	delete buffer;
}

// TODO: safety checks :)
void CYDecoder::ProcessLine(const char* line)
{
	//=ybegin part=10 line=128 size=5120000 name=VW Sharan-Technik.part1.rar
	//=ypart begin=2304001 end=2560000
	//=yend size=256000 part=10 pcrc32=D183E6B9
	if(!strncmp(line, "=ybegin ", strlen("=ybegin "))) {
		const char* nameBegin = strstr(line, "name=") + strlen("name=");
		const char* nameEnd = strpbrk(nameBegin, "\r\n");
		fileName = CString("temp\\") + CString(nameBegin, nameEnd - nameBegin);
	} else if(!strncmp(line, "=ypart", strlen("=ypart"))) {
		const char* beginStr = strstr(line, "begin=") + strlen("begin=");
		offset = _strtoi64(beginStr, NULL, 10) - 1;
		const char* endStr = strstr(line, "end=") + strlen("end=");
		size = (unsigned int)(_strtoi64(endStr, NULL, 10) - 1 - offset + 1);
		buffer = ptr = new char [size];
	} else if(!strncmp(line, "=yend ", strlen("=yend "))) {
		CDiskWriter::Instance()->Add(fileName, offset, buffer, size);
		buffer = NULL; // buffer is now managed by disk writer
	} else {
		unsigned char lineBuf[1024], *out = lineBuf;
		while(*line != '\r' && *line != '\n') {
			unsigned char c = *line++;
			if(c == '=') {
				c = *line++;
				c = (unsigned char)(c-64);
			}
			c = (unsigned char)(c-42);
			*ptr++ = c;
		}
	}
}

// CDownloader
//////////////////////////////////////////////////////////////////////////

static CString GetIni(LPCTSTR section, LPCTSTR key, LPCTSTR def)
{
	CString val;
	DWORD ret = GetPrivateProfileString(section, key, def, val.GetBuffer(1024), 1024, _T(".\\server.ini"));
	val.ReleaseBuffer(ret);

	return val;
}

DWORD CDownloader::Run()
{
	sock.Connect(GetIni(_T("Server"), _T("Host"), _T("localhost")), GetIni(_T("Server"), _T("Port"), _T("119")));
	CStringA reply = sock.ReceiveLine(); Util::print(reply);
	sock.Send("AUTHINFO USER %s\n", (const char*)CStringA(GetIni(_T("Server"), _T("User"), _T("xxx")))); reply = sock.ReceiveLine(); Util::print(reply);
	sock.Send("AUTHINFO PASS %s\n", (const char*)CStringA(GetIni(_T("Server"), _T("Password"), _T("xxx")))); reply = sock.ReceiveLine(); Util::print(reply);
	CString curGroup;
	while(CNzbSegment* s = CNewzflow::Instance()->GetSegment()) {
		CNzbFile* f = s->parent;
		if(f->groups[0] != curGroup) {
			sock.Send("GROUP %s\n", CStringA(f->groups[0])); reply = sock.ReceiveLine(); Util::print(reply);
			curGroup = f->groups[0];
		}
		sock.Send("ARTICLE <%s>\n", CStringA(s->msgId)); 
//		CFile fout;
//		fout.Open(CString("temp\\") + f->segments[j]->msgId, GENERIC_WRITE, 0, CREATE_ALWAYS);
		reply = sock.ReceiveLine();  Util::print(reply);
		bool noBody = false;
		// skip headers
		while(1) {
			reply = sock.ReceiveLine(); 
			if(reply.GetLength() > 0 && (reply[0] == '\n' || reply[0] == '\r'))
				break;
			if(reply.GetLength() >= 2 && reply[0] == '.' && reply[1] != '.') {
				noBody = true;
				break;
			}
		};
		// process body
		if(!noBody) {
			CYDecoder yd;
			while(1) {
				reply = sock.ReceiveLine(); 
				if(reply.GetLength() >= 2 && reply[0] == '.') {
					if(reply[1] == '.')
						reply = reply.Mid(1); // special case: '.' at beginning of line is duplicated
					else
						break; // special case: '.' at begining of line: end of article
				}
//				fout.Write(reply, reply.GetLength());
				yd.ProcessLine(reply);
			}
//			fout.Close();
		}
		CNewzflow::Instance()->UpdateSegment(s, kCompleted);
	}
	sock.Send("QUIT\n"); reply = sock.ReceiveLine(); Util::print(reply);
	sock.Close();

	return 0;
}