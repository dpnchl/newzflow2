#pragma once

// forwards
class CParSet;
class CNzbFile;
class CNzb;

// 'char' consts for better readability of queue.dat and to make sure that enums don't change when new consts are added
enum ENzbStatus {
	kEmpty = 'e',
	kFetching = 'f',
	kQueued = 'Q',
	kPaused = 'P',
	kDownloading = 'D',
	kCached = 'c',
	kCompleted = 'C',
	kError = 'E',
	kPostProcessing = 'O',
	kFinished = 'F',
};

enum EPostProcStatus {
	kVerifying = 'V',
	kJoining = 'J',
	kRepairing = 'R',
	kUnpacking = 'U',
};

enum EParStatus {
	kUnknown,
	kScanning,
	kMissing,
	kFound,
	kDamaged,
};

CString GetNzbStatusString(ENzbStatus status, EPostProcStatus postProcStatus = kVerifying, float done = 0.f, int setDone = 0, int setTotal = 0);
CString GetParStatusString(EParStatus status, float done);

class CParFile {
public:
	CParFile(CParSet* _parent) {
		parent = _parent;
	}

	CNzbFile* file;
	int numBlocks;
	CParSet* parent;
};

class CParSet {
public:
	CParSet(CNzb* _parent) {
		parent = _parent;
		needBlocks = 0;
		completed = false;
	}
	~CParSet() {
		for(size_t i = 0; i < pars.GetCount(); i++) delete pars[i];
	}
	CString baseName;
	CAtlArray<CParFile*> pars;
	CAtlArray<CNzbFile*> files; // just a reference to target files of par set (information retrieved from par2 tool)
	CNzb* parent;

	int needBlocks;
	bool completed;
};

class CNzbSegment {
public:
	CNzbSegment(CNzbFile* _parent) {
		parent = _parent;
		status = kQueued;
	}

	ENzbStatus status;

	unsigned __int64 bytes; // from nzb
	int number; // from nzb
	CString msgId; // from nzb
	CNzbFile* parent;

/*	Segment Status flow chart:

		kQueued: default state
		||
		|| @CNewzflow::GetSegment()
		\/
		kDownloading: a downloader is assigned to download this segment
	.===||
	|	||
	|	\/
	|	kCached: all segments of this file have either been downloaded or errored
	|	||
	|	|| [write segment to TEMP dir]
	|	\/
	|
	`==>kError: Error downloading or decoding this segment
*/
};

class CNzbFile {
public:
	CNzbFile(CNzb* _parent) {
		parent = _parent;
		status = kQueued;
		parStatus = kUnknown;
	}
	~CNzbFile() {
		for(size_t i = 0; i < segments.GetCount(); i++) delete segments[i];
	}
	void Init(); // parses subject into filename and calculates segments' offsets and filesize;

	ENzbStatus status;
	EParStatus parStatus;
	float parDone;

	CString subject; // from nzb
	CString fileName;
	unsigned __int64 size;
	CAtlArray<CString> groups; // from nzb
	CAtlArray<CNzbSegment*> segments; // from nzb
	CNzb* parent;

/*	File Status flow chart:

	kQueued: default state
	||
	|| @CNewzflow::GetSegment()
	\/
	         busy downloading segments from this file
	||
	||
	\/
	kCached: all segments of this file have either been downloaded or errored
	||
	|| @CDiskWriter::OnJob() [Diskwriter assembles segments of this file and writes it to the destination directory]
	\/
	kCompleted: file has been assembled on disk
*/
};

// don't access anything else than status and done when status == kEmpty or status == kFetching
class CNzb {
public:
	CNzb() {
		status = kEmpty;
		done = 0.f;
		setDone = setTotal = 0;
		refCount = 0;
		doRepair = doUnpack = doDelete = true;
		CoCreateGuid(&guid); // create GUID so we can later store the queue and just reference the GUID and copy file to %appdata%\Newzflow
	}
	~CNzb() {
		for(size_t i = 0; i < files.GetCount(); i++) delete files[i];
		for(size_t i = 0; i < parSets.GetCount(); i++) delete parSets[i];
	}

public:
	bool CreateFromPath(const CString& path); // add new NZB to queue
	bool CreateFromQueue(REFGUID guid, const CString& name); // restore from queue
	bool CreateFromLocal();
	bool SetPath(LPCTSTR _path, LPCTSTR _name, int* errorCode); // set the storage path to path\name (name=NULL => use nzb->name)

	CString GetLocalPath(); // location where .nzb is stored locally in %APPDATA%
	void Cleanup(); // deletes local .nzb file and part files

	CNzbFile* FindFile(const CString& name);

protected:
	void Init();
	bool Parse(const CString& path);

public:
	ENzbStatus status;
	EPostProcStatus postProcStatus;
	int setDone, setTotal; // used for status = [kVerifying, kUnpacking]
	float done; // percentage completed; used for status = [kVerifying, kUnpacking]

	GUID guid;
	CString name;
	CString path;
	bool doRepair, doUnpack, doDelete;
	CAtlArray<CNzbFile*> files; // from nzb
	CAtlArray<CParSet*> parSets;

	int refCount;

/*
	NZB Status flow chart:

				.==<kEmpty: CNzb has just been created and added to the queue, but hasn't been parsed yet
				|   ||
				|   || [http downloader]
				|	\/
	[created	V	kFetching: NZB file is currently being fetched from HTTP/HTTPS
	 from File	|	||
	 or Queue]	|	|| [ParseNZB]
				|	\/
				`==>kPaused: NZB has just been parsed
				    ||
					|| (if no download directory is specified in Settings, ask CMainFrame to query the user for a directory)
					|| [SetPath]
					\/
				.==>kQueued: NZB has a download directory
				|	||
	[need more	^	|| [downloading in progress]
	 PAR files]	|	\/
				|	kCompleted: All Files of the NZB are either completed, paused (PAR files), or errored
				|	||
				`===|| [postprocessor: PAR2 verify/repair]
					\/
					kUnpacking: All PAR2 files have been processed (either repaired or not)
					||
					|| [postprocessor: RAR unpacking]
					\/
					kFinished: Processing completed
*/
};
