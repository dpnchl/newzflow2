#pragma once

// forwards
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

class CNzbSegment {
public:
	CNzbSegment(CNzbFile* _parent) {
		parent = _parent;
		status = kQueued;
	}

	ENzbStatus status;

	unsigned __int64 bytes;
	int number;
	CString msgId;
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

	ENzbStatus status;
	EParStatus parStatus;
	float parDone;

	CString subject; // from .nzb-file
	CString fileName; // yEnc decoder fills in fileName here
	CAtlArray<CString> groups;
	CAtlArray<CNzbSegment*> segments;
	CNzb* parent;
};

class CNzb {
public:
	CNzb() {
		status = kQueued;
		done = 0.f;
		refCount = 0;
		ZeroMemory(&guid, sizeof(GUID));
	}
	~CNzb() {
		for(size_t i = 0; i < files.GetCount(); i++) delete files[i];
	}

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
	CAtlArray<CNzbFile*> files;

	int refCount;
};
