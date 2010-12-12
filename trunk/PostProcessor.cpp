#include "stdafx.h"
#include "util.h"
#include "PostProcessor.h"
#include "nzb.h"
#include "Newzflow.h"
#include "Settings.h"
#include "NntpSocket.h"
#include "Compress.h"
#include "resource.h"
#include "MainFrm.h"

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
	{ NEWZFLOW_LOCK;
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
	nzb->setTotal = nzb->parSets.GetCount();
	for(size_t i = 0; i < nzb->parSets.GetCount(); i++) {
		CParSet* parSet = nzb->parSets[i];
		if(parSet->completed)
			continue;
		// only process this par set if at least 1 par file has been downloaded
		CParFile* parFile = NULL;
		for(int j = (int)parSet->pars.GetCount() - 1; j >= 0; j--) {
			CParFile* pf = parSet->pars[j];
			if(pf->file->status == kCompleted && (j == 0 || CFile::GetFileSize(pf->parent->parent->path + pf->file->fileName) > 0)) {
				parFile = pf;
				break;
			}
		}
		// try to repair
		if(parFile) {
			nzb->setDone = i+1;
			Par2Repair(parFile);
		}
	}

	bool finished = true;
	{
		NEWZFLOW_LOCK;
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
						// go from largest to smallest PAR2 file so we don't download more than necessary
						for(int j = (int)parSet->pars.GetCount() - 1; j >= 0 && parSet->needBlocks > 0; j--) {
							CParFile* parFile = parSet->pars[j];
							if(parFile->file->status == kPaused && parSet->needBlocks >= parFile->numBlocks) {
								parFile->file->status = kQueued;
								parSet->needBlocks = max(0, parSet->needBlocks - parFile->numBlocks);
							}
						}
						// now go from smallest to largest PAR2 file -- this should only happen if there are missing PAR2 files
						for(size_t j = 0; j < parSet->pars.GetCount() && parSet->needBlocks > 0; j++) {
							CParFile* parFile = parSet->pars[j];
							if(parFile->file->status == kPaused) {
								parFile->file->status = kQueued;
								parSet->needBlocks = max(0, parSet->needBlocks - parFile->numBlocks);
							}
						}
						ASSERT(parSet->needBlocks == 0);
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
			nzb->status = kPostProcessing;
			nzb->postProcStatus = kUnpacking;
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

		CAtlArray<std::pair<CNzbFile*, CString> > rars;

		// get all RarSets
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

			CString sBase;
			if(hasPart)
				sBase = fn.Left(fn.GetLength() - match[1].length());
			else
				sBase = fn;

			rars.Add(std::make_pair(file, sBase));
		}

		// process all RarSets
		nzb->setTotal = rars.GetCount();
		for(size_t i = 0; i < rars.GetCount(); i++) {
			CNzbFile* file = rars[i].first;
			CString sBase = rars[i].second;

			nzb->setDone = i+1;
			time_t timeStart = time(NULL);
			if(UncompressRAR(file)) {
				CString s; s.Format(_T("%s: Unpacked in %s\r\n"), file->fileName, Util::FormatTimeSpan(time(NULL) - timeStart)); nzb->log += s;
				// unrar succeeded, so delete all rar files of thie rar set
				for(size_t j = 0; j < nzb->files.GetCount(); j++) {
					CNzbFile* file2 = nzb->files[j];
					if(!sBase.CompareNoCase(file2->fileName.Left(sBase.GetLength()))) {
						CFile::Delete(nzb->path + _T("\\") + file2->fileName);
						//TRACE(_T("Delete(%s)\n"), nzb->path + file2->fileName);
					}
				}
			} else {
				CString s; s.Format(_T("%s: Unpacking failed\r\n"), file->fileName); nzb->log += s;
			}
		}
		NEWZFLOW_LOCK;
		nzb->status = kFinished;
		Util::GetMainWindow().PostMessage(CMainFrame::MSG_NZB_FINISHED, (WPARAM)nzb); // handler decreases nzb->refCount

		CNewzflow::Instance()->WriteQueue();
	}
	currentNzb = NULL;

	return 0;
}

// verify & repair par2file
// if allowJoin, split files are detected, joined, and par2file is verified & repaired again
// (used to prevent infinite recursion)
void CPostProcessor::Par2Repair(CParFile* par2file, bool allowJoin /*= true*/)
{
	CNzb* nzb = par2file->parent->parent;

	// try a quick check first
	if(allowJoin && !par2file->parent->quickCheckFailed) {
		if(Par2QuickCheck(par2file)) {
			CString s; s.Format(_T("%s.par2: Quick check succeeded\r\n"), par2file->parent->baseName); nzb->log += s;
			Util::Print("Par2QuickCheck succeeded");
			Par2Cleanup(par2file->parent);
			return;
		} else {
			par2file->parent->quickCheckFailed = true; // so we don't try again
			CString s; s.Format(_T("%s.par2: Quick check failed\r\n"), par2file->parent->baseName); nzb->log += s;
			Util::Print("Par2QuickCheck failed");
		}
	}

	time_t timeStart = time(NULL);
	CExternalTool tool;
	CString cmdline;
	cmdline.Format(_T("%s\\par2.exe r \"%s\\%s\""), CNewzflow::Instance()->settings->GetProgramDir(), nzb->path, par2file->file->fileName);
	if(!tool.Run(cmdline))
		return;

	par2file->parent->files.RemoveAll();

	bool repairRequired = false;
	bool repairPossible = false;
	bool repairing = false;
	int numFiles = 0, numFilesDone = 0;

	using std::tr1::tregex; using std::tr1::tcmatch; using std::tr1::regex_search; using std::tr1::regex_match;

	tregex reNumFiles(_T("There are (\\d+) recoverable files"));
	tregex reNeedMore(_T("You need (\\d+) more recovery blocks"));
	tregex reScanning(_T("Scanning: \"([^\"]+)\": ([0-9]+(\\.[0-9]+)?)%"));
	tregex reMatrix(_T("Computing Reed Solomon matrix"));
	tregex reRepairing(_T("Repairing: ([0-9]+(\\.[0-9]+)?)%"));
	tregex reTarget(_T("Target: \"([^\"]+)\" - (.*)+$"));
	tcmatch match;

	typedef CAtlArray<std::pair<CNzbFile*, int> > CSplitArray;
	CAtlArray<CSplitArray*> splits;

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
				} else {
					// file not in nzb
					if(status.Find(_T("missing")) >= 0) {
						if(allowJoin) {
							// check if we have split versions of the file
							tregex reSplit(_T("\\.(\\d{3})$"));

							CSplitArray* split = new CSplitArray;
							for(size_t i = 0; i < nzb->files.GetCount(); i++) {
								CNzbFile* file = nzb->files[i];
								if(regex_search((const TCHAR*)file->fileName, match, reSplit)) {
									split->Add(std::make_pair(file, _ttoi(match[1].first)));
								}
							}
							if(!split->IsEmpty()) {
								struct splitSorter {
									bool operator()(const std::pair<CNzbFile*, int>& _Left, const std::pair<CNzbFile*, int>& _Right) const
									{
										return (_Left.second < _Right.second);
									}
								};
								std::sort(&split->GetData()[0], &split->GetData()[split->GetCount()], splitSorter());
								splits.Add(split);
							} else {
								delete split;
							}
						}
					}
				}
				numFilesDone++;
				if(numFiles > 0) {
					nzb->done = 100.f * (float)numFilesDone / (float)numFiles;
				}
			} else if(regex_search((const TCHAR*)line, match, reMatrix)) {
				nzb->status = kPostProcessing;
				nzb->postProcStatus = kRepairing;
				nzb->done = 0.f;
				repairing = true;
			} else if(regex_match((const TCHAR*)line, match, reRepairing)) {
				float percent = (float)_tstof(match[1].first);
				nzb->status = kPostProcessing;
				nzb->postProcStatus = kRepairing;
				nzb->done = percent;
				repairing = true;
			} else {
				if(line.Find(_T("Verifying source files")) >= 0) {
					nzb->status = kPostProcessing;
					nzb->postProcStatus = kVerifying;
					nzb->done = 0.f;
					numFilesDone = 0;
				} else if(line.Find(_T("Verifying repaired files")) >= 0) {
					nzb->status = kPostProcessing;
					nzb->postProcStatus = kVerifying;
					nzb->done = 0.f;
					numFilesDone = 0;
				}
				else if(line.Find(_T("Repair is required")) >= 0) repairRequired = true;
				else if(line.Find(_T("Repair is possible")) >= 0) repairPossible = true;
				else if(line.Find(_T("Repair complete")) >= 0) {
					nzb->done = 100.f;
					CString s; s.Format(_T("%s.par2: Repaired in %s\n"), par2file->parent->baseName, Util::FormatTimeSpan(time(NULL) - timeStart)); nzb->log += s;
					Par2Cleanup(par2file->parent);
				} else if(line.Find(_T("repair is not required")) >= 0) {
					nzb->done = 100.f;
					CString s; s.Format(_T("%s.par2: Verified in %s\r\n"), par2file->parent->baseName, Util::FormatTimeSpan(time(NULL) - timeStart)); nzb->log += s;
					Par2Cleanup(par2file->parent);
				} else if(line.Find(_T("Main packet not found")) >= 0 || line.Find(_T("The recovery file does not exist")) >= 0) {
					par2file->parent->needBlocks = 1;
				}
			}
		}
	}

	if(!splits.IsEmpty()) {
		// count split files for progress
		nzb->status = kPostProcessing;
		nzb->postProcStatus = kJoining;
		nzb->done = 0.f;

		__int64 totalBytes = 0;
		__int64 doneBytes = 0;
		for(size_t i = 0; i < splits.GetCount(); i++) {
			CSplitArray& split = *splits[i];
			for(size_t j = 0; j < split.GetCount(); j++) {
				CString splitPath = nzb->path + _T("\\") + split[j].first->fileName;
				totalBytes += CFile::GetFileSize(splitPath);
			}
		}

		if(totalBytes > 0) {
			// join split files
			for(size_t i = 0; i < splits.GetCount(); i++) {
				time_t timeStart = time(NULL);
				CSplitArray& split = *splits[i];
				CString joinedName = split[0].first->fileName.Left(split[0].first->fileName.GetLength() - 4);
				CString joinedPath = nzb->path + _T("\\") + joinedName;
				CFile joinedFile;
				if(joinedFile.Open(joinedPath, GENERIC_WRITE, 0, CREATE_ALWAYS)) { // TODO: Error check
					for(size_t j = 0; j < split.GetCount(); j++) {
						CFile splitFile;
						CString splitPath = nzb->path + _T("\\") + split[j].first->fileName;
						if(splitFile.Open(splitPath, GENERIC_READ, 0, OPEN_ALWAYS)) {
							// TODO: limit buffer size to 4MB
							int size = (int)splitFile.GetSize();
							char* buffer = new char [size];
							splitFile.Read(buffer, size);
							splitFile.Close();
							CFile::Delete(splitPath); // TODO: do only if writing everything succeeded
							joinedFile.Write(buffer, size);
							delete buffer;
							doneBytes += size;
							nzb->done = 100.f * (float)doneBytes / (float)totalBytes;
						}
					}
					joinedFile.Close();
				}
				CString s; s.Format(_T("%s: Joined %d parts in %s\r\n"), joinedName, split.GetCount(), Util::FormatTimeSpan(time(NULL) - timeStart)); nzb->log += s;
				delete splits[i];
			}
			Par2Repair(par2file, false);
		}
	} else {
		if(par2file->parent->needBlocks) {
			CString s; s.Format(_T("%s.par2: Need %d more blocks\r\n"), par2file->parent->baseName, par2file->parent->needBlocks); nzb->log += s;
		}
	}

	TRACE(_T("PostProcessing finished\n"));
}

// QuickCheck works by comparing the MD5 of each file created in CDiskWriter with the MD5 stored in the PAR2 file
// If the PAR2 file seems corrupt, this will fail, and the normal Par2Repair will continue
// see http://parchive.sourceforge.net/docs/specifications/parity-volume-spec/article-spec.html for a description of the PAR2 file format
bool CPostProcessor::Par2QuickCheck(CParFile* par2file)
{
	CNzb* nzb = par2file->parent->parent;

	nzb->status = kPostProcessing;
	nzb->postProcStatus = kQuickCheck;
	nzb->done = 0.f;

	CFile f;
	if(!f.Open(nzb->path + _T("\\") + par2file->file->fileName))
		return false;

	DWORD read;

	int correctFiles = 0;

	for(;;) {
		char header[8];
		if(!f.Read(header, 8, &read) || read != 8)
			break;

		if(memcmp(header, "PAR2\0PKT", 8))
			return false;

		unsigned __int64 packetLength;
		if(!f.Read(&packetLength, 8, &read) || read != 8)
			return false;

		if((packetLength & 3) || packetLength < 20)
			return false;

		// skip packets larger than 32kb (can't be FileDesc packets)
		if(packetLength > 32768) {
			f.Seek(packetLength - 16, FILE_CURRENT);
			continue;
		}

		char md5Packet[16];
		if(!f.Read(&md5Packet, 16, &read) || read != 16)
			return false;

		char* data = new char [(size_t)packetLength - 32];
		if(!f.Read(data, (size_t)packetLength - 32, &read) || read != packetLength - 32) {
			delete data;
			return false;
		}

		if(memcmp(&data[16], "PAR 2.0\0FileDesc", 16)) {
			delete data;
			continue;
		}

		// check MD5 of PAR Packet
		MD5_CTX md5;
		MD5Init(&md5);
		MD5Update(&md5, data, (size_t)packetLength - 32);
		MD5Final(&md5);

		if(memcmp(md5.digest, md5Packet, 16)) {
			delete data;
			return false;
		}

		char* md5File = &data[48];
		unsigned __int64 nameLen = *(unsigned __int64*)&data[80];
		char* nameFile = &data[88];

		CStringA nameString(nameFile, (size_t)packetLength - 32 - 88);
		// find target file in NZB
		CNzbFile* file = nzb->FindFile(CString(nameString));
		if(!file) {
			delete data;
			return false;
		}

		// check MD5 of target file
		if(memcmp(md5File, file->md5.digest, 16)) {
			file->parStatus = kDamaged;
			delete data;
			return false;
		}

		correctFiles++;
		file->parStatus = kFound;

		delete data;
	}

	nzb->done = 100.f;

	return correctFiles > 0;
}


void CPostProcessor::Par2Cleanup(CParSet* parSet)
{
	// delete all par2 files
	for(size_t i = 0; i < parSet->pars.GetCount(); i++) {
		CNzbFile* file = parSet->pars[i]->file;
		CFile::Delete(file->parent->path + _T("\\") + file->fileName);
	}
	// delete all backups of repaired target files (.1 ending)
	for(size_t i = 0; i < parSet->files.GetCount(); i++) {
		CNzbFile* file = parSet->files[i];
		CFile::Delete(file->parent->path + _T("\\") + file->fileName + _T(".1"));
	}
}

bool CPostProcessor::UncompressRAR(CNzbFile* file)
{
	CNzb* nzb = file->parent;
	nzb->done = 0;
	CString rarfile(nzb->path + _T("\\") + file->fileName);

	return Compress::UnRar(rarfile, nzb->path, &nzb->done);
}
