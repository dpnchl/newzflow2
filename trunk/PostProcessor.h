#pragma once

class CNzb;
class CNzbFile;
class CParSet;
class CParFile;
class CLineBuffer;

// run external tool and capture tool's stdout. Get output one line at a time
// one-time use only
// if(Run(cmdLine)) {
//   while(Process()) {
//     while(!GetLine().IsEmpty()) {
//       ...
//     }
//   }
// }

class CExternalTool
{
public:
	CExternalTool();
	~CExternalTool();

	bool Run(const CString& cmdLine);
	bool Process();
	CString GetLine();

protected:
	CHandle hStdOutRd, hStdOutWr;
	CLineBuffer* buffer;
};

class CPostProcessor : public CGuiThreadImpl<CPostProcessor>
{
	enum {
		MSG_JOB = WM_USER+1,
	};

	BEGIN_MSG_MAP(CPostProcessor)
		MESSAGE_HANDLER(MSG_JOB, OnJob)
	END_MSG_MAP()

public:
	CPostProcessor() : CGuiThreadImpl<CPostProcessor>(&_Module) {}

	void Add(CNzb* nzb);
	LRESULT OnJob(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	void Par2Repair(CParFile* par2file);
	void Par2Cleanup(CParSet* parSet);
	bool Unrar(CNzbFile* file);
	__int64 GetRarTotalSize(const CString& rarfile);
};