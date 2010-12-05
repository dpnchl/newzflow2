#pragma once

class CDirWatcher : public CThreadImpl<CDirWatcher>
{
public:
	CDirWatcher();

	DWORD Run();

	void ProcessDirectory(const CString& dir);

protected:
	void ReadIgnoreList();
	void WriteIgnoreList();

	CAtlArray<CString> filesToIgnore;
};