#pragma once

// forwards
class CNzbFile;
class CNzb;

enum ENzbStatus {
	kQueued,
	kPaused,
	kDownloading,
	kCompleted,
	kError,
	kVerifying,
	kFinished,
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
	}
	~CNzb() {
		for(size_t i = 0; i < files.GetCount(); i++) delete files[i];
	}

	CNzbFile* FindByName(const CString& name);

	ENzbStatus status;
	float done; // percentage completed; used for status = [kVerifying]

	CString name;
	CAtlArray<CNzbFile*> files;

	static CNzb* Create(const CString& path);
};
