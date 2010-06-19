#pragma once

class CNzb;
class CNzbFile;

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

	void Par2Verify(CNzbFile* par2file);
};