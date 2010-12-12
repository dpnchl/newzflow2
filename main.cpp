// Newzflow.cpp : main source file for Newzflow.exe
//

#include "stdafx.h"

#include "resource.h"

#include "MainFrm.h"
#include "Newzflow.h"
#include "Settings.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

CAppModule _Module;
CString _CmdLine;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	// TODO: make optional
	Util::RegisterAssoc(_Module.GetModuleInstance());

	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	int nRet;
	{
		CMainFrame wndMain;

		CRect windowRect = CNewzflow::Instance()->settings->GetWindowPos(nCmdShow);

		if(wndMain.CreateEx(NULL, !windowRect.IsRectEmpty() ? &windowRect : NULL) == NULL)
		{
			ATLTRACE(_T("Main window creation failed!\n"));
			return 0;
		}

		if(!_CmdLine.IsEmpty())
			CNewzflow::Instance()->controlThread->AddFile(_CmdLine);

		wndMain.ShowWindow(nCmdShow);

		nRet = theLoop.Run();
	}

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	_CmdLine = lpstrCmdLine;
	if(!_CmdLine.IsEmpty() && _CmdLine[0] == '\"') _CmdLine = _CmdLine.Mid(1);
	if(!_CmdLine.IsEmpty() && _CmdLine[_CmdLine.GetLength()-1] == '\"') _CmdLine = _CmdLine.Left(_CmdLine.GetLength()-1);

	// check if another instance is already running; pass argument to running instance and terminate self
CheckOtherInstance:
	HANDLE mutex = ::CreateMutex(NULL, FALSE, _T("Newzflow-{F583B0CF-96BA-4f21-86AD-E6317A53E692}"));
	if(GetLastError() == ERROR_ALREADY_EXISTS || GetLastError() == ERROR_ACCESS_DENIED) {
		// find other main window; this could fail, if other instance is just starting up, or shutting down, so we keep trying for a bit
		for(int i = 0; i < 10; i++) {
			CWindow wnd = FindWindow(_T("NewzflowMainFrame"), NULL);
			if(wnd) {
				if(!_CmdLine.IsEmpty()) {
					COPYDATASTRUCT cds;
					cds.lpData = (PVOID)(LPCTSTR)_CmdLine;
					cds.cbData = _CmdLine.GetLength() * sizeof(TCHAR);
					wnd.SendMessage(WM_COPYDATA, NULL, (LPARAM)&cds); // could block/hang when the receiving instance is busy, but we don't care. It will probably wake up sometimes...
				} else {
					SetForegroundWindow(wnd);
					if(IsIconic(wnd))
						ShowWindow(wnd, SW_RESTORE);
				}
				return 0;
			}
			Sleep(500);
		}
		// if we reach here, it means we couldn't find the main window of the other instance in 10 seconds, probably crashed,
		// so try creating the mutex once again. If other instance crashed, we could become the new instance now
		goto CheckOtherInstance;
	}

	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES | ICC_LINK_CLASS); // add flags to support other controls
	HINSTANCE hInstRich = ::LoadLibrary(CRichEditCtrl::GetLibraryName());

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet;
	{
		CNewzflow theApp;
		nRet = Run(lpstrCmdLine, nCmdShow);
	}

	_Module.Term();
	::FreeLibrary(hInstRich);
	::CoUninitialize();

	return nRet;
}
