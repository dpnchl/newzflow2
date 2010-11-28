#include "stdafx.h"
#include "util.h"
#include "nzb.h"
#include "Downloader.h"
#include "DiskWriter.h"
#include "NntpSocket.h"
#include "nzb.h"
#include "Newzflow.h"
#include "Settings.h"

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

// TODO: safety checks!! :)
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
		ptr = NULL;
	} else {
		if(ptr) {
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
}

// CDownloader
//////////////////////////////////////////////////////////////////////////

CDownloader::CDownloader()
: CThreadImpl<CDownloader>(CREATE_SUSPENDED)
{
	connected = false;
	shutDown = false;

	Resume();
}

DWORD CDownloader::Run()
{
	int penalty = 0; // in seconds
	CNzbSegment* segment = NULL;

	for(;;) {
		if(CNewzflow::Instance()->IsShuttingDown() || shutDown)
			break;

		if(penalty > 0) {
			sock.SetLastCommand(_T("Waiting %s..."), Util::FormatTimeSpan(penalty));
			Sleep(1000);
			penalty--;
			continue;
		}

		// get a segment to download
		if(!segment) {
			segment = CNewzflow::Instance()->GetSegment();
			if(!segment) break;
		}

		if(!connected) {
			EStatus status = Connect();
			if(status == kTemporaryError) {
				penalty = 10; // can't connect; wait 10 seconds before trying again
				continue;
			} else if(status == kPermanentError) {
				break;
			}
		}

		EStatus status = DownloadSegment(segment);
		if(status == kTemporaryError) {
			penalty = 10; // error during download process; wait 10 seconds before trying again
			Disconnect();
			continue;
		}
		// kSuccess and kPermanentError are already handled in DownloadSegment, so we don't need to do anything here
		segment = NULL;
	}
	// permanent error; return segment to queue
	if(segment)
		CNewzflow::Instance()->UpdateSegment(segment, kQueued);

	Disconnect();

	CNewzflow::Instance()->RemoveDownloader(this);
	return 0;
}

CDownloader::EStatus CDownloader::Connect()
{
	curGroup.Empty();
	connected = false;
	CSettings* settings = CNewzflow::Instance()->settings;
	if(!sock.Connect(
		settings->GetIni(_T("Server"), _T("Host"), _T("localhost")), 
		settings->GetIni(_T("Server"), _T("Port"), _T("119")), 
		settings->GetIni(_T("Server"), _T("User"), NULL), 
		settings->GetIni(_T("Server"), _T("Password"), NULL))) {
			return kTemporaryError;
	}
	CStringA reply = sock.ReceiveLine();
	// error has occured
	if(reply.Left(3) != "200") {
		// try to disconnect if we received any reply at all
		if(!reply.IsEmpty())
			Disconnect();
		return kTemporaryError;
	}
	connected = true;
	return kSuccess;
}

void CDownloader::Disconnect()
{
	sock.Request("QUIT\n");
	sock.Close();
	connected = false;
}

CDownloader::EStatus CDownloader::DownloadSegment(CNzbSegment* segment)
{
	CStringA reply;
	CNzbFile* file = segment->parent;
	ENzbStatus status = kError;
	CYDecoder yd;
	for(size_t i = 0; i < file->groups.GetCount(); i++) {
		const CString& group = file->groups[i];
		if(group != curGroup) {
			reply = sock.Request("GROUP %s\n", CStringA(group));
			if(reply.IsEmpty())
				return kTemporaryError;
			if(reply.Left(3) != "211") // group doesn't exist, try next
				continue;
			curGroup = group;
		}
		reply = sock.Request("ARTICLE <%s>\n", CStringA(segment->msgId));
		if(reply.IsEmpty())
			return kTemporaryError;
		if(reply.Left(3) == "220") {
			status = kDownloading;
			break;
		}
	}
	// if status is still kError here, it means that none of the specified groups exist or the article does not exist
	if(status == kDownloading) {
		//CFile fout;
		//fout.Open(CString("temp\\") + f->segments[j]->msgId, GENERIC_WRITE, 0, CREATE_ALWAYS);
		bool noBody = false;
		// skip headers
		while(1) {
			reply = sock.ReceiveLine(); 
			if(reply.IsEmpty())
				return kTemporaryError;
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
			while(1) {
				reply = sock.ReceiveLine(); 
				if(reply.IsEmpty())
					return kTemporaryError;
				if(reply.GetLength() >= 2 && reply[0] == '.') {
					if(reply[1] == '.')
						reply = reply.Mid(1); // special case: '.' at beginning of line is duplicated
					else {
						status = kCached;
						break; // special case: '.' at begining of line: end of article
					}
				}
				//fout.Write(reply, reply.GetLength());
				yd.ProcessLine(reply);
			}
			if(file->fileName.IsEmpty())
				file->fileName = yd.fileName;
			else if(file->fileName != yd.fileName) {
				TRACE(_T("segment has different filename than file! %s != %s\n"), file->fileName, yd.fileName);
			}
		}
		//fout.Close();
	}
	// write out segment to temp dir
	if(status == kCached) {
		CString nzbDir;
		nzbDir.Format(_T("%s%s"), CNewzflow::Instance()->settings->GetAppDataDir(), CComBSTR(segment->parent->parent->guid));
		CreateDirectory(nzbDir, NULL);
		CString segFileName;
		segFileName.Format(_T("%s\\%s_part%05d"), nzbDir, segment->parent->fileName, segment->number);
		CFile fout;
		fout.Open(segFileName, GENERIC_WRITE, 0, CREATE_ALWAYS);
		fout.Write(yd.buffer, yd.size);
		fout.Close();
	}
	CNewzflow::Instance()->UpdateSegment(segment, status);
	ASSERT(status == kCached || status == kError);
	if(status == kCached)
		return kSuccess;
	if(status == kError)
		return kPermanentError;

	// never reaches here
	return kPermanentError;
}
