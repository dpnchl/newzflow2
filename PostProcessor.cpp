#include "stdafx.h"
#include "util.h"
#include "PostProcessor.h"
#include "nzb.h"
#include "Newzflow.h"
#include "sock.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

// CExternalTool
//////////////////////////////////////////////////////////////////////////

CExternalTool::CExternalTool()
{
	buffer = NULL;
}

CExternalTool::~CExternalTool()
{
	delete buffer;
}

bool CExternalTool::Run(const CString& cmdLine)
{
	SECURITY_ATTRIBUTES saAttr; 

	// Set the bInheritHandle flag so pipe handles are inherited. 
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL; 

	// Create a pipe for the child process's STDOUT. 
	VERIFY(CreatePipe(&hStdOutRd.m_h, &hStdOutWr.m_h, &saAttr, 0));

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	VERIFY(SetHandleInformation(hStdOutRd, HANDLE_FLAG_INHERIT, 0));

	// Create the child process. 
	CString szCmdline(cmdLine);
	PROCESS_INFORMATION piProcInfo; 
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE; 

	// Set up members of the PROCESS_INFORMATION structure. 
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO); 
	siStartInfo.hStdError = hStdOutWr;
	siStartInfo.hStdOutput = hStdOutWr;
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
	if(!bSuccess)
		return false;

	// Close handles to the child process and its primary thread.
	// Some applications might keep these handles to monitor the status
	// of the child process, for example. 
	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);

	// Close the write end of the pipe before reading from the 
	// read end of the pipe, to control child process execution.
	// The pipe is assumed to have enough buffer space to hold the
	// data the child process has already written to it.
	hStdOutWr.Close();

	buffer = new CLineBuffer(4096);

	return true;
}

bool CExternalTool::Process()
{
	ASSERT(buffer);

	// Read from pipe
	DWORD dwRead;

	BOOL bSuccess = ReadFile(hStdOutRd, buffer->GetFillPtr(), buffer->GetFillSize(), &dwRead, NULL);
	if(!bSuccess || dwRead == 0) 
		return false;

	buffer->Fill(NULL, dwRead);
	return true;
}

CString CExternalTool::GetLine()
{
	ASSERT(buffer);

	return buffer->GetLine();
}

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
	Par2Repair(par2file);

	return 0;
}

void CPostProcessor::Par2Repair(CNzbFile* par2file)
{
	CNzb* nzb = par2file->parent;

	CExternalTool tool;
	CString cmdline;
	cmdline.Format(_T("test\\par2\\par2.exe r \"temp\\%s\""), par2file->fileName);
	if(!tool.Run(cmdline))
		return;

	bool repairRequired = false;
	bool repairPossible = false;
	bool repairing = false;
	int needBlocks = 0;
	int numFiles = 0, numFilesDone = 0;

	std::tr1::tregex reNumFiles(_T("There are (\\d+) recoverable files"));
	std::tr1::tregex reNeedMore(_T("You need (\\d+) more recovery blocks"));
	std::tr1::tregex reScanning(_T("Scanning: \"([^\"]+)\": ([0-9]+(\\.[0-9]+)?)%"));
	std::tr1::tregex reMatrix(_T("Computing Reed Solomon matrix"));
	std::tr1::tregex reRepairing(_T("Repairing: ([0-9]+(\\.[0-9]+)?)%"));
	std::tr1::tregex reTarget(_T("Target: \"([^\"]+)\" - (.*)+$"));
	std::tr1::tcmatch match;

	while(tool.Process()) {
		CString line;
		while(!(line = tool.GetLine()).IsEmpty()) {
			//TRACE(_T(">%s<\n"), line);
			if(std::tr1::regex_search((const TCHAR*)line, match, reNumFiles)) {
				numFiles = _ttoi(match[1].first);
				//TRACE(_T("***%d files***\n"), numFiles);
			} else if(std::tr1::regex_search((const TCHAR*)line, match, reNeedMore)) {
				needBlocks = _ttoi(match[1].first);
			} else if(std::tr1::regex_match((const TCHAR*)line, match, reScanning)) {
				CString fn(match[1].first, match[1].length());
				float percent = (float)_tstof(match[2].first);
				//TRACE(_T("***%s*** %f\n"), fn, percent);
				CNzbFile* file = nzb->FindByName(fn);
				if(file) {
					file->parStatus = kScanning;
					file->parDone = percent;
				}
				if(numFiles > 0) {
					nzb->done = 100.f * (float)numFilesDone / (float)numFiles + percent / (float)numFiles;
				}
			} else if(std::tr1::regex_match((const TCHAR*)line, match, reTarget)) {
				CString fn(match[1].first, match[1].length());
				CString status(match[2].first, match[2].length());
				//TRACE(_T("***%s*** %s\n"), fn, status);
				CNzbFile* file = nzb->FindByName(fn);
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
					nzb->done = 100.f * (float)numFilesDone / (float)numFiles;
				}
			} else if(std::tr1::regex_search((const TCHAR*)line, match, reMatrix)) {
				nzb->status = kRepairing;
				nzb->done = 0.f;
				repairing = true;
			} else if(std::tr1::regex_match((const TCHAR*)line, match, reRepairing)) {
				float percent = (float)_tstof(match[1].first);
				nzb->status = kRepairing;
				nzb->done = percent;
				repairing = true;
			} else {
				if(line.Find(_T("Verifying source files")) >= 0) {
					nzb->status = kVerifying;
					nzb->done = 0.f;
					numFilesDone = 0;
				} else if(line.Find(_T("Verifying repaired files")) >= 0) {
					nzb->status = kVerifying;
					nzb->done = 0.f;
					numFilesDone = 0;
				}
				else if(line.Find(_T("Repair is required")) >= 0) repairRequired = true;
				else if(line.Find(_T("Repair is possible")) >= 0) repairPossible = true;
				else if(line.Find(_T("Repair complete")) >= 0) {
					nzb->done = 100.f;
				}
			}
		}
	} 
}
