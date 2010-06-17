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
};

CString GetNzbStatusString(ENzbStatus status);

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
	}
	~CNzbFile() {
		for(size_t i = 0; i < segments.GetCount(); i++) delete segments[i];
	}

	ENzbStatus status;

	CString subject;
	CAtlArray<CString> groups;
	CAtlArray<CNzbSegment*> segments;
	CNzb* parent;
};

class CNzb {
public:
	CNzb() {
		status = kQueued;
	}
	~CNzb() {
		for(size_t i = 0; i < files.GetCount(); i++) delete files[i];
	}

	ENzbStatus status;

	CString name;
	CAtlArray<CNzbFile*> files;

	static CNzb* Create(const CString& path);
};
