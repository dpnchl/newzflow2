#include "stdafx.h"
#include "util.h"
#include "PostProcessor.h"
#include "nzb.h"
#include "Newzflow.h"
#include "sock.h"

// CPostProcessor
//////////////////////////////////////////////////////////////////////////

void CPostProcessor::Add(CNzb* nzb)
{
	PostThreadMessage(MSG_JOB, (WPARAM)nzb);
}

LRESULT CPostProcessor::OnJob(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CNzb* nzb = (CNzb*)wParam;

	CNzbFile* par2file = NULL;
	for(size_t i = 0; i < nzb->files.GetCount(); i++) {
		CNzbFile* file = nzb->files[i];
		CString fn(file->fileName); fn.MakeLower();
		if(file->fileName.Find(_T(".par2")) >= 0) {
			par2file = file;
			break;
		}
	}
	if(!par2file) {
		nzb->status = kFinished;
		return 0;
	}
	nzb->status = kVerifying;
	Par2Verify(par2file);

	return 0;
}

void CPostProcessor::Par2Verify(CNzbFile* par2file)
{
	HANDLE g_hChildStd_IN_Rd = NULL;
	HANDLE g_hChildStd_IN_Wr = NULL;
	HANDLE g_hChildStd_OUT_Rd = NULL;
	HANDLE g_hChildStd_OUT_Wr = NULL;

	SECURITY_ATTRIBUTES saAttr; 

	// Set the bInheritHandle flag so pipe handles are inherited. 
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL; 

	// Create a pipe for the child process's STDOUT. 
	VERIFY(CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0));

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	VERIFY(SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0));

	// Create a pipe for the child process's STDIN. 
	VERIFY(CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0));

	// Ensure the write handle to the pipe for STDIN is not inherited. 
	VERIFY(SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0));

	// Create the child process. 
	CString szCmdline;
	szCmdline.Format(_T("test\\par2\\par2.exe v \"temp\\%s\""), par2file->fileName);
	PROCESS_INFORMATION piProcInfo; 
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE; 

	// Set up members of the PROCESS_INFORMATION structure. 
	ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.

	ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
	siStartInfo.cb = sizeof(STARTUPINFO); 
	siStartInfo.hStdError = g_hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
	siStartInfo.hStdInput = g_hChildStd_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	// Create the child process. 
	bSuccess = CreateProcess(NULL, 
		szCmdline.GetBuffer(),     // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		CREATE_NO_WINDOW,  // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION 
	szCmdline.ReleaseBuffer();

	// If an error occurs, exit the application. 
	ASSERT(bSuccess);

	// Close handles to the child process and its primary thread.
	// Some applications might keep these handles to monitor the status
	// of the child process, for example. 
	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);

	// Read from pipe
#define BUFSIZE 4096
	DWORD dwRead;
	bSuccess = FALSE;

	// Close the write end of the pipe before reading from the 
	// read end of the pipe, to control child process execution.
	// The pipe is assumed to have enough buffer space to hold the
	// data the child process has already written to it.
	VERIFY(CloseHandle(g_hChildStd_OUT_Wr));

	CFile f;
	f.Open(_T("par2.dmp"), GENERIC_WRITE, 0, CREATE_ALWAYS);

	CLineBuffer buffer(BUFSIZE);

	bool repairRequired = false;
	bool repairPossible = false;
	int numFiles = 0, numFilesDone = 0;

	std::tr1::tregex reNumFiles(_T("There are (\\d+) recoverable files"));
	std::tr1::tregex reScanning(_T("Scanning: \"([^\"]+)\": ([0-9]+(\\.[0-9]+)?)%"));
	std::tr1::tregex reTarget(_T("Target: \"([^\"]+)\" - (.*)+$"));
	std::tr1::tcmatch match;

	for (;;) 
	{ 
		bSuccess = ReadFile(g_hChildStd_OUT_Rd, buffer.GetFillPtr(), buffer.GetFillSize(), &dwRead, NULL);
		if( ! bSuccess || dwRead == 0 ) break; 

		buffer.Fill(NULL, dwRead);
		CString line;
		do {
			line = buffer.GetLine();
			//TRACE(_T(">%s<\n"), line);
			if(std::tr1::regex_search((const TCHAR*)line, match, reNumFiles)) {
				numFiles = _ttoi(match[1].first);
				//TRACE(_T("***%d files***\n"), numFiles);
			} else if(std::tr1::regex_match((const TCHAR*)line, match, reScanning)) {
				CString fn(match[1].first, match[1].length());
				float percent = (float)_tstof(match[2].first);
				//TRACE(_T("***%s*** %f\n"), fn, percent);
				CNzbFile* file = par2file->parent->FindByName(fn);
				if(file) {
					file->parStatus = kScanning;
					file->parDone = percent;
				}
				if(numFiles > 0) {
					par2file->parent->done = 100.f * (float)numFilesDone / (float)numFiles + percent / 100.f / (float)numFiles;
				}
			} else if(std::tr1::regex_match((const TCHAR*)line, match, reTarget)) {
				CString fn(match[1].first, match[1].length());
				CString status(match[2].first, match[2].length());
				//TRACE(_T("***%s*** %s\n"), fn, status);
				CNzbFile* file = par2file->parent->FindByName(fn);
				if(file) {
					file->parDone = 100.f;
					if(status.Find(_T("missing")) >= 0)
						file->parStatus = kMissing;
					else if(status.Find(_T("found")) >= 0)
						file->parStatus = kFound;
					else if(status.Find(_T("damaged")) >= 0) // "damaged. Found %d of %d data blocks"; TODO: parse number
						file->parStatus = kDamaged;
				}
				numFilesDone++;
				if(numFiles > 0) {
					par2file->parent->done = 100.f * (float)numFilesDone / (float)numFiles;
				}
			} else {
				if(line.Find(_T("Repair is required") >= 0)) repairRequired = true;
				else if(line.Find(_T("Repair is possible") >= 0)) repairPossible = true;
			}
		} while(!line.IsEmpty());
	} 
}
