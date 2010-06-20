#include "stdafx.h"
#include "util.h"
#include "DownloadQueue.h"
#include "Downloader.h"
#include "DiskWriter.h"
#include "sock.h"
#include "nzb.h"
#include "Newzflow.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

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
		fileName = CString(nameBegin, nameEnd - nameBegin);
	} else if(!strncmp(line, "=ypart", strlen("=ypart"))) {
		const char* beginStr = strstr(line, "begin=") + strlen("begin=");
		offset = _strtoi64(beginStr, NULL, 10) - 1;
		const char* endStr = strstr(line, "end=") + strlen("end=");
		size = (unsigned int)(_strtoi64(endStr, NULL, 10) - 1 - offset + 1);
		buffer = ptr = new char [size];
	} else if(!strncmp(line, "=yend ", strlen("=yend "))) {
		CNewzflow::Instance()->diskWriter->Add(CString("temp\\") + fileName, offset, buffer, size);
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
	sock.Connect(
		GetIni(_T("Server"), _T("Host"), _T("localhost")), 
		GetIni(_T("Server"), _T("Port"), _T("119")),
		GetIni(_T("Server"), _T("User"), NULL),
		GetIni(_T("Server"), _T("Password"), NULL)
	);
	CStringA reply = sock.ReceiveLine();
	if(reply.Left(3) != "200")
		return 1;

	CString curGroup;
	while(CNzbSegment* s = CNewzflow::Instance()->GetSegment()) {
		CNzbFile* f = s->parent;
		ENzbStatus status = kError;
		for(size_t i = 0; i < f->groups.GetCount(); i++) {
			const CString& group = f->groups[i];
			if(group != curGroup) {
				reply = sock.Request("GROUP %s\n", CStringA(group));
				if(reply.Left(3) != "211") {
					continue;
				}
				curGroup = group;
			}
			reply = sock.Request("ARTICLE <%s>\n", CStringA(s->msgId));
			if(reply.Left(3) == "220") {
				status = kDownloading;
				break;
			}
		}
		if(status == kDownloading) {
			//CFile fout;
			//fout.Open(CString("temp\\") + f->segments[j]->msgId, GENERIC_WRITE, 0, CREATE_ALWAYS);
			bool noBody = false;
			// skip headers
			while(1) {
				reply = sock.ReceiveLine(); 
				if(reply.IsEmpty()) {
					status = kError;
					break;
				}
				if(reply.GetLength() > 0 && (reply[0] == '\n' || reply[0] == '\r'))
					break;
				if(reply.GetLength() >= 2 && reply[0] == '.' && reply[1] != '.') {
					noBody = true;
					break;
				}
			};
			// process body
			if(noBody) {
				status = kError;
			} else {
				CYDecoder yd;
				while(1) {
					reply = sock.ReceiveLine(); 
					if(reply.IsEmpty()) {
						status = kError;
						break;
					}
					if(reply.GetLength() >= 2 && reply[0] == '.') {
						if(reply[1] == '.')
							reply = reply.Mid(1); // special case: '.' at beginning of line is duplicated
						else {
							status = kCompleted;
							break; // special case: '.' at begining of line: end of article
						}
					}
					//fout.Write(reply, reply.GetLength());
					yd.ProcessLine(reply);
				}
				if(f->fileName.IsEmpty())
					f->fileName = yd.fileName;
				else if(f->fileName != yd.fileName) {
					TRACE(_T("segment has different filename than file! %s != %s\n"), f->fileName, yd.fileName);
				}
			}
			//fout.Close();
		}
		CNewzflow::Instance()->UpdateSegment(s, status);
	}
	sock.Request("QUIT\n");
	sock.Close();

	CNewzflow::Instance()->RemoveDownloader(this);

	return 0;
}