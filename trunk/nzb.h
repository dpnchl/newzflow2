#pragma once

// forwards
class CParSet;
class CNzbFile;
class CNzb;

// 'char' consts for better readability of queue.dat and to make sure that enums don't change when new consts are added
enum ENzbStatus {
	kQueued = 'Q',
	kPaused = 'P',
	kDownloading = 'D',
	kCached = 'c',
	kCompleted = 'C',
	kError = 'E',
	kVerifying = 'V',
	kRepairing = 'R',
	kUnpacking = 'U',
	kFinished = 'F',
};

enum EParStatus {
	kUnknown,
	kScanning,
	kMissing,
	kFound,
	kDamaged,
};

CString GetNzbStatusString(ENzbStatus status, float done = 0.f);
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
	kDownloading: busy downloading segments from this file
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

class CNzb {
public:
	CNzb() {
		status = kQueued;
		done = 0.f;
		refCount = 0;
		doRepair = doUnpack = doDelete = true;
		path = _T("temp\\");
		ZeroMemory(&guid, sizeof(GUID));
	}
	~CNzb() {
		Cleanup();
		for(size_t i = 0; i < files.GetCount(); i++) delete files[i];
		for(size_t i = 0; i < parSets.GetCount(); i++) delete parSets[i];
	}
	void Init();
	void Cleanup();

public:
	CNzbFile* FindByName(const CString& name);

	static CNzb* Create(const CString& path); // add new NZB to queue
	static CNzb* Create(REFGUID guid, const CString& name); // restore from queue
protected:
	static CNzb* ParseNZB(const CString& path);

public:
	ENzbStatus status;
	float done; // percentage completed; used for status = [kVerifying]

	GUID guid;
	CString name;
	CString path;
	bool doRepair, doUnpack, doDelete;
	CAtlArray<CNzbFile*> files; // from nzb
	CAtlArray<CParSet*> parSets;

	int refCount;

/*
	NZB Status flow chart:

				.==>kQueued: NZB has just been created/added
				|	||
				|	|| [downloading in progress]
				|	\/
	[need more	|	kDownloading: Files are currently being downloaded
	 PAR files]	|	||
				|	||
				|	\/
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
