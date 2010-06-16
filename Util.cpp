#include "stdafx.h"
#include "util.h"
#include <fcntl.h>
#include <io.h>

namespace Util
{
	void CreateConsole()
	{
		// redirect unbuffered STDOUT to the console
		int hConHandle;
		long lStdHandle;
		FILE *fp;
		AllocConsole();
		lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
		hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
		fp = _fdopen( hConHandle, "w" );
		*stdout = *fp;
		setvbuf( stdout, NULL, _IONBF, 0 );
		// redirect unbuffered STDIN to the console
		lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
		hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
		fp = _fdopen( hConHandle, "r" );
		*stdin = *fp;
		setvbuf( stdin, NULL, _IONBF, 0 );
		// redirect unbuffered STDERR to the console
		lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
		hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
		fp = _fdopen( hConHandle, "w" );
		*stderr = *fp;
		setvbuf( stderr, NULL, _IONBF, 0 );
	}

	void print(const char* s)
	{
/*
		GUITHREADINFO gui;
		ZeroMemory(&gui, sizeof(gui));
		gui.cbSize = sizeof(gui);
		GetGUIThreadInfo(_Module.m_dwMainThreadID, &gui);
		::PostMessage(gui.hwndActive, WM_USER, (WPARAM)new CString(s), (LPARAM)::GetCurrentThreadId());
*/
		CString str;
		str.Format(_T("[%4x] %s\n"), GetCurrentThreadId(), CString(s));
		OutputDebugString(str);
	}

	CString FormatSize(__int64 size)
	{
		static TCHAR prefix[] = _T("KMGT");
		CString s;
		if(size < 1024)
			s.Format(_T("%I64 B"), size);
		else {
			int idx = -1;
			double dsize = (double)size;
			while(dsize >= 1024.0 && idx+1 < sizeof(prefix)/sizeof(prefix[0])) {
				idx++;
				dsize /= 1024.0;
			}
			if(dsize < 10.0)
				s.Format(_T("%.2f %cB"), dsize, prefix[idx]);
			else if(dsize < 100.0)
				s.Format(_T("%.1f %cB"), dsize, prefix[idx]);
			else
				s.Format(_T("%.0f %cB"), dsize, prefix[idx]);
		}
		return s;
	}
};
