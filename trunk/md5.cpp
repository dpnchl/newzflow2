#include "stdafx.h"
#include "md5.h"

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

MD5INIT MD5Init;
MD5UPDATE MD5Update;
MD5FINAL MD5Final;

namespace {
	class CMD5Loader {
	public:
		CMD5Loader() {
			HMODULE hLib = LoadLibrary(_T("cryptdll.Dll"));
			MD5Init = (MD5INIT)GetProcAddress(hLib, "MD5Init");
			MD5Update = (MD5UPDATE)GetProcAddress(hLib, "MD5Update");
			MD5Final = (MD5FINAL)GetProcAddress(hLib, "MD5Final");
		}
	};
	static CMD5Loader _theLoader;
}
