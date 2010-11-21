/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Util.h"
#include "Compress.h"
#include "zlib/zlib.h"
#include "zlib/unzip.h"
#include "zlib/iowin32.h"
#include "unrar.h"
#pragma comment(lib, "unrar.lib")

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

namespace Compress {

__int64 GetZipTotalSize(const CString& zipfile)
{
	__int64 totalSize = 0;

	zlib_filefunc64_def ffunc; 
	fill_win32_filefunc64(&ffunc); // unicode support depends on UNICODE define

	unzFile uf = unzOpen2_64(zipfile, &ffunc);
	if(!uf)
		return totalSize;

	unz_global_info64 gi;
	int err = unzGetGlobalInfo64(uf, &gi);
	if(err != UNZ_OK) {
		unzClose(uf);
		return totalSize;
	}

	for(int i = 0; i < gi.number_entry; i++, unzGoToNextFile(uf)) {
		unz_file_info64 file_info;
		err = unzGetCurrentFileInfo64(uf, &file_info, NULL, 0, NULL, 0, NULL, 0); 
		if(err != UNZ_OK)
			break;
		totalSize += file_info.uncompressed_size;
	}

	unzClose(uf);

	return totalSize;
}

bool UnZip(const CString& zipfile, const CString& _dstPath, float* progress)
{
	CString dstPath(_dstPath);
	if(dstPath.Right(1) != _T("\\")) dstPath += '\\';

	__int64 totalSize = GetZipTotalSize(zipfile);
	__int64 done = 0;
	if(progress)
		*progress = 0.f;

	zlib_filefunc64_def ffunc; 
	fill_win32_filefunc64(&ffunc); // unicode support depends on UNICODE define

	unzFile uf = unzOpen2_64(zipfile, &ffunc);
	if(!uf)
		return false;

	unz_global_info64 gi;
	int err = unzGetGlobalInfo64(uf, &gi);
	if(err != UNZ_OK) {
		unzClose(uf);
		return false;
	}

	for(int i = 0; i < gi.number_entry; i++, unzGoToNextFile(uf)) {
		unz_file_info64 file_info;
	    char filename_inzip[512];
		err = unzGetCurrentFileInfo64(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0); 
		CString filename;
		if(file_info.flag & 2048) { // UTF8 encoded filename
			::MultiByteToWideChar(CP_UTF8, 0, filename_inzip, -1, filename.GetBuffer(512), 512);
			filename.ReleaseBuffer();
		} else {
			filename = filename_inzip;
		}
		if(err != UNZ_OK) {
			unzClose(uf);
			return false;
		}
		filename.Replace('/', '\\');
		if(filename.Right(1) == _T("\\")) {
			CreateDirectory(dstPath + filename, NULL);
			continue;
		}
/*
		int slash = filename.ReverseFind('/');
		if(slash < 0) slash = filename.ReverseFind('\\');
		CString fileNoDir;
		if(slash >= 0) 
			fileNoDir = filename.Mid(slash + 1);
		else
			fileNoDir = filename;

		// skip if it's a directory
		if(fileNoDir.IsEmpty())
			continue;
*/
		CFile fout;
		if(!fout.Open(dstPath + filename, GENERIC_WRITE, 0, CREATE_ALWAYS)) {
			unzClose(uf);
			return false;
		}

		err = unzOpenCurrentFile(uf);
		char* buffer = new char [64*1024];
		do {
			err = unzReadCurrentFile(uf, buffer, 64*1024);
			if(err < 0) {
				delete[] buffer;
				unzClose(uf);
				return false;
			}
			if(err > 0) {
				fout.Write(buffer, err);
				done += err;
				if(progress)
					*progress = 100.f * (float)done / (float)totalSize;
			}
		} while(err > 0);
		delete[] buffer;
		fout.Close();
		unzCloseCurrentFile(uf);
	}

	unzClose(uf);

	return false;
}

namespace {

struct RarProgress {
	__int64 totalSize;
	__int64 done;
	float* percentage;
};

int CALLBACK RarCallbackProc(UINT msg, LPARAM UserData, LPARAM P1, LPARAM P2)
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
		if(progress->percentage)
			*progress->percentage = 100.f * (float)progress->done / (float)progress->totalSize;
		return 0;
	}
	case UCM_NEEDPASSWORD:
		strcpy((char*)P1, "xxxxx"); // we don't have a password
		return 0;
	}
	return 0;
}

__int64 GetRarTotalSize(const CString& rarfile)
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

} // namespace

// unpacks rarfile to dstPath
// outputs progress in percent
bool UnRar(const CString& rarfile, const CString& dstPath, float* progress)
{
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

	RarProgress rarprogress;
	rarprogress.totalSize = totalSize;
	rarprogress.done = 0;
	rarprogress.percentage = progress;

	RARSetCallback(hArcData, RarCallbackProc, (LPARAM)&rarprogress);

	bool ret = true;

	{ CString s; s.Format(_T("UncompressRAR(%s)\n"), rarfile); Util::Print(s); }

	while((RHCode = RARReadHeaderEx(hArcData, &HeaderData)) == 0) {
		{ CString s; s.Format(_T("  %s\n"), HeaderData.FileNameW); Util::Print(s); }
		if ((PFCode = RARProcessFileW(hArcData, RAR_EXTRACT, (wchar_t*)(const wchar_t*)dstPath, NULL)) != 0) {
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

} // namespace Compress