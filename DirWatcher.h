#pragma once

class CDirWatcher : public CThreadImpl<CDirWatcher>
{
public:
	CDirWatcher();

	DWORD Run();

	void ProcessDirectory(const CString& dir);

	CEvent shutDown; // CNewzflow will set this event to indicate the dir watcher should shut down

protected:
	void ReadIgnoreList();
	void WriteIgnoreList();

	CAtlArray<CString> filesToIgnore;
};