#pragma once

// forwards
class CParSet;
class CNzbFile;
class CNzb;

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
};
