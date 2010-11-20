#include "stdafx.h"
#include "util.h"
#include "PostProcessor.h"
#include "nzb.h"
#include "Newzflow.h"
#include "NntpSocket.h"
#include "unrar.h"
#pragma comment(lib, "unrar.lib")

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
/*
	CFile f;
	f.Open(_T("externalTool.txt"), GENERIC_WRITE, 0, CREATE_ALWAYS);
	f.SetEOF();

#ifdef _UNICODE
	unsigned char bom[] = { 0xff, 0xfe };
	f.Write(bom, 2);
#endif
*/
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
/*
	CFile f;
	f.Open(_T("externalTool.txt"), GENERIC_WRITE, OPEN_EXISTING);
	f.Seek(0, FILE_END);*/
	CString s = buffer->GetLine();
/*	f.Write(s, s.GetLength() * sizeof(TCHAR));
	TCHAR cr = '\n';
	f.Write(&cr, sizeof(TCHAR));
*/
	return s;
}

// CPostProcessor
//////////////////////////////////////////////////////////////////////////

void CPostProcessor::Add(CNzb* nzb)
{
	{ CNewzflow::CLock lock;
		nzb->refCount++;
	}

	PostThreadMessage(MSG_JOB, (WPARAM)nzb);
}

LRESULT CPostProcessor::OnJob(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CNzb* nzb = (CNzb*)wParam;
	currentNzb = nzb;

	CNzbFile* par2file = NULL;
	// process all par sets
	for(size_t i = 0; i < nzb->parSets.GetCount(); i++) {
		CParSet* parSet = nzb->parSets[i];
		if(parSet->completed)
			continue;
		// only process this par set if at least 1 par file has been downloaded
		CParFile* parFile = NULL;
		for(size_t j = 0; j < parSet->pars.GetCount(); j++) {
			CParFile* pf = parSet->pars[j];
			if(pf->file->status == kCompleted) {
				parFile = pf;
				break;
			}
		}
		// try to repair
		if(parFile) {
			Par2Repair(parFile);
		}
	}

	bool finished = true;
	{
		CNewzflow::CLock lock;
		nzb->refCount--;

		// now unpause needed par files
		for(size_t i = 0; i < nzb->parSets.GetCount(); i++) {
			CParSet* parSet = nzb->parSets[i];
			if(!parSet->completed) {
				if(parSet->needBlocks > 0) {
					// first check if paused available blocks would suffice
					int couldBlocks = 0;
					for(size_t j = 0; j < parSet->pars.GetCount(); j++) {
						CParFile* pf = parSet->pars[j];
						if(pf->file->status == kPaused)
							couldBlocks += pf->numBlocks;
					}
					// if there are enough left, unpause as needed
					// TODO: how can we handle when unpaused PAR2s are corrupted and don't have as many blocks as we expected?
					if(couldBlocks >= parSet->needBlocks) {
						for(int j = (int)parSet->pars.GetCount() - 1; j >= 0 && parSet->needBlocks > 0; j--) {
							CParFile* parFile = parSet->pars[j];
							if(parFile->file->status == kPaused && parSet->needBlocks >= parFile->numBlocks) {
								parFile->file->status = kQueued;
								parSet->needBlocks = max(0, parSet->needBlocks - parFile->numBlocks);
							}
						}
						finished = false; // requeue nzb
					} else {
						// not enough available, so we might just skip requeuing at all
						// TODO: or should we get all available anyway?
						parSet->completed = true;
					}
				} else {
					parSet->completed = true;
				}
			}
		}

		if(finished) {
			nzb->status = kUnpacking;
			nzb->refCount++;
		} else
			nzb->status = kQueued;

		CNewzflow::Instance()->WriteQueue();
		if(!finished)
			CNewzflow::Instance()->CreateDownloaders();
	}
	// TODO: don't unpack if PAR repair failed; need to get relation between par2 and rar-files
	if(finished) {
		// get RarSets
		//   can be either filename.rar->filename.r00->filename.r01->...
		//   or filename.part0001.rar->filename.part0002.rar->...
		using std::tr1::tregex; using std::tr1::tcmatch; using std::tr1::regex_search; using std::tr1::regex_match;
		tregex rePart(_T("\\.part(\\d+)$"));
		tcmatch match;

		for(size_t i = 0; i < nzb->files.GetCount(); i++) {
			CNzbFile* file = nzb->files[i];
			CString fn(file->fileName);
			fn.MakeLower();
			// check if file is the first in a set of rar files
			if(fn.Right(4) != _T(".rar"))
				continue;
			fn = fn.Left(fn.GetLength() - 4);
			bool hasPart = regex_search((const TCHAR*)fn, match, rePart);
			// it's the first part if name contains ".part[0]*1.rar" or just ".rar" (without "part")
			if(hasPart && _ttoi(CString(match[1].first, match[1].length())) != 1)
				continue;
			if(Unrar(file)) {
				// unrar succeeded, so delete all rar files of thie rar set
				CString sBase;
				if(hasPart)
					sBase = fn.Left(fn.GetLength() - match[1].length());
				else
					sBase = fn;
				for(size_t j = 0; j < nzb->files.GetCount(); j++) {
					CNzbFile* file2 = nzb->files[j];
					if(!sBase.CompareNoCase(file2->fileName.Left(sBase.GetLength()))) {
						CFile::Delete(nzb->path + file2->fileName);
						//TRACE(_T("Delete(%s)\n"), nzb->path + file2->fileName);
					}
				}
			}
		}
		CNewzflow::CLock lock;
		nzb->status = kFinished;
		nzb->refCount--;
		CNewzflow::Instance()->WriteQueue();
	}
	currentNzb = NULL;

	return 0;
}

void CPostProcessor::Par2Repair(CParFile* par2file)
{
	CNzb* nzb = par2file->parent->parent;

	CExternalTool tool;
	CString cmdline;
	cmdline.Format(_T("test\\par2\\par2.exe r \"%s%s\""), nzb->path, par2file->file->fileName);
	if(!tool.Run(cmdline))
		return;

	par2file->parent->files.RemoveAll();

	bool repairRequired = false;
	bool repairPossible = false;
	bool repairing = false;
	int needBlocks = 0;
	int numFiles = 0, numFilesDone = 0;

	using std::tr1::tregex; using std::tr1::tcmatch; using std::tr1::regex_search; using std::tr1::regex_match;

	tregex reNumFiles(_T("There are (\\d+) recoverable files"));
	tregex reNeedMore(_T("You need (\\d+) more recovery blocks"));
	tregex reScanning(_T("Scanning: \"([^\"]+)\": ([0-9]+(\\.[0-9]+)?)%"));
	tregex reMatrix(_T("Computing Reed Solomon matrix"));
	tregex reRepairing(_T("Repairing: ([0-9]+(\\.[0-9]+)?)%"));
	tregex reTarget(_T("Target: \"([^\"]+)\" - (.*)+$"));
	tcmatch match;

	while(tool.Process()) {
		CString line;
		while(!(line = tool.GetLine()).IsEmpty()) {
			//TRACE(_T(">%s<\n"), line);
			if(regex_search((const TCHAR*)line, match, reNumFiles)) {
				numFiles = _ttoi(match[1].first);
				//TRACE(_T("***%d files***\n"), numFiles);
			} else if(regex_search((const TCHAR*)line, match, reNeedMore)) {
				par2file->parent->needBlocks = _ttoi(match[1].first);
			} else if(regex_match((const TCHAR*)line, match, reScanning)) {
				CString fn(match[1].first, match[1].length());
				float percent = (float)_tstof(match[2].first);
				//TRACE(_T("***%s*** %f\n"), fn, percent);
				CNzbFile* file = nzb->FindFile(fn);
				if(file) {
					file->parStatus = kScanning;
					file->parDone = percent;
				}
				if(numFiles > 0) {
					nzb->done = 100.f * (float)numFilesDone / (float)numFiles + percent / (float)numFiles;
				}
			} else if(regex_match((const TCHAR*)line, match, reTarget)) {
				CString fn(match[1].first, match[1].length());
				CString status(match[2].first, match[2].length());
				//TRACE(_T("***%s*** %s\n"), fn, status);
				CNzbFile* file = nzb->FindFile(fn);
				if(file) {
					par2file->parent->files.Add(file); // add target file to par2 set
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
			} else if(regex_search((const TCHAR*)line, match, reMatrix)) {
				nzb->status = kRepairing;
				nzb->done = 0.f;
				repairing = true;
			} else if(regex_match((const TCHAR*)line, match, reRepairing)) {
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
					Par2Cleanup(par2file->parent);
				} else if(line.Find(_T("repair is not required")) >= 0) {
					nzb->done = 100.f;
					Par2Cleanup(par2file->parent);
				}
			}
		}
	} 
	TRACE(_T("PostProcessing finished\n"));
}

void CPostProcessor::Par2Cleanup(CParSet* parSet)
{
	// delete all par2 files
	for(size_t i = 0; i < parSet->pars.GetCount(); i++) {
		CNzbFile* file = parSet->pars[i]->file;
		CFile::Delete(file->parent->path + file->fileName);
	}
	// delete all backups of repaired target files (.1 ending)
	for(size_t i = 0; i < parSet->files.GetCount(); i++) {
		CNzbFile* file = parSet->files[i];
		CFile::Delete(file->parent->path + file->fileName + _T(".1"));
	}
}

struct RarProgress {
	__int64 totalSize;
	__int64 done;
	float* percentage;
};

static int CALLBACK RarCallbackProc(UINT msg, LPARAM UserData, LPARAM P1, LPARAM P2);

bool CPostProcessor::Unrar(CNzbFile* file)
{
	CNzb* nzb = file->parent;
	nzb->done = 0;
	CString rarfile(nzb->path + file->fileName);

	__int64 totalSize = GetRarTotalSize(rarfile);

	HANDLE hArcData;
	int RHCode, PFCode;
	struct RARHeaderDataEx HeaderData;
	struct RAROpenArchiveDataEx OpenArchiveData;

	memset(&OpenArchiveData, 0, sizeof(OpenArchiveData));
	memset(&HeaderData, 0, sizeof(HeaderData));
	OpenArchiveData.ArcNameW = (wchar_t*)(const wchar_t*)rarfile;
	OpenArchiveData.OpenMode = RAR_OM_EXTRACT;
	hArcData = RAROpenArchiveEx(&OpenArchiveData);

	if(OpenArchiveData.OpenResult != 0)
		return false;

	RarProgress progress;
	progress.totalSize = totalSize;
	progress.done = 0;
	progress.percentage = &nzb->done;

	RARSetCallback(hArcData, RarCallbackProc, (LPARAM)&progress);

	bool ret = true;

	{ CString s; s.Format(_T("CPostProcessor::Unrar(%s)\n"), rarfile); Util::Print(s); }

	while((RHCode = RARReadHeaderEx(hArcData, &HeaderData)) == 0) {
		{ CString s; s.Format(_T("  %s\n"), HeaderData.FileNameW); Util::Print(s); }
		if ((PFCode = RARProcessFileW(hArcData, RAR_EXTRACT, (wchar_t*)(const wchar_t*)nzb->path, NULL)) != 0) {
			Util::Print("   failed\n");
			ret = false;
			break;
		}
	}

	if (RHCode == ERAR_BAD_DATA)
		ret = false;

	RARCloseArchive(hArcData);

	return ret;
}

__int64 CPostProcessor::GetRarTotalSize(const CString& rarfile)
{
	__int64 totalSize = 0;

	HANDLE hArcData;
	int RHCode, PFCode;
	struct RARHeaderDataEx HeaderData;
	struct RAROpenArchiveDataEx OpenArchiveData;

	memset(&OpenArchiveData, 0, sizeof(OpenArchiveData));
	memset(&HeaderData, 0, sizeof(HeaderData));
	OpenArchiveData.ArcNameW = (wchar_t*)(const wchar_t*)rarfile;
	OpenArchiveData.OpenMode = RAR_OM_LIST;
	hArcData = RAROpenArchiveEx(&OpenArchiveData);

	if(OpenArchiveData.OpenResult != 0) {
		//OutOpenArchiveError(OpenArchiveData.OpenResult,ArcName);
		return 0;
	}

	RARSetCallback(hArcData, RarCallbackProc, 0);

	while((RHCode = RARReadHeaderEx(hArcData, &HeaderData)) == 0) {
		__int64 UnpSize = HeaderData.UnpSize + (((__int64)HeaderData.UnpSizeHigh) << 32);
		totalSize += UnpSize;
		if ((PFCode = RARProcessFile(hArcData, RAR_SKIP, NULL, NULL)) != 0) {
			//OutProcessFileError(PFCode);
			break;
		}
	}

	RARCloseArchive(hArcData);

	return totalSize;
}

static int CALLBACK RarCallbackProc(UINT msg, LPARAM UserData, LPARAM P1, LPARAM P2)
{
	switch(msg) {
	case UCM_CHANGEVOLUME:
		if (P2 == RAR_VOL_ASK) {
			// we don't have any other volumes
			return -1;
		}
		return 0;
	case UCM_PROCESSDATA: {
		RarProgress* progress = (RarProgress*)UserData;
		progress->done += P2;
		*progress->percentage = 100.f * (float)progress->done / (float)progress->totalSize;
		return 0;
	}
	case UCM_NEEDPASSWORD:
		strcpy((char*)P1, "xxxxx"); // we don't have a password
		return 0;
	}
	return 0;
}
