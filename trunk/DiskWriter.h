#pragma once

class CDiskWriter : public CGuiThreadImpl<CDiskWriter>
{
	enum {
		MSG_JOB = WM_USER+1,
	};

	BEGIN_MSG_MAP(CDiskWriter)
		MESSAGE_HANDLER(MSG_JOB, OnJob)
	END_MSG_MAP()

public:
	CDiskWriter() : CGuiThreadImpl<CDiskWriter>(&_Module) {}

	void Add(const CString& file, __int64 offset, void* buffer, unsigned int size);
	LRESULT OnJob(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

protected:
	class CJob {
	public:
		CString file;
		__int64 offset;
		void* buffer;
		unsigned int size;
	};
};