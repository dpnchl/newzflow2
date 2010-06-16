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
	CDiskWriter() : CGuiThreadImpl<CDiskWriter>(&_Module)
	{
		ASSERT(s_pInstance == NULL);
		s_pInstance = this;
	}
	~CDiskWriter()
	{
		s_pInstance = NULL;
	}
	static CDiskWriter* Instance()
	{
		return s_pInstance;
	}

	void Add(const CString& file, __int64 offset, void* buffer, unsigned int size);
	LRESULT OnJob(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

protected:
	static CDiskWriter* s_pInstance;

	class CJob {
	public:
		CString file;
		__int64 offset;
		void* buffer;
		unsigned int size;
	};
};