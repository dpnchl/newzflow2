// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once

// Change these values to use different versions
#define WINVER			0x0500
#define _WIN32_WINNT	0x0502
#define _WIN32_IE		0x0501
#define _RICHEDIT_VER	0x0200

#define PP_FILL 5
#define PBFS_NORMAL 1
#define PBFS_ERROR 2


#define ASSERT ATLASSERT
#define TRACE ATLTRACE
#ifdef _DEBUG
	#define VERIFY(f) ASSERT(f)
#else
	#define VERIFY(f) ((void)(f))
#endif

#define _CRT_SECURE_NO_WARNINGS
#define _WTL_NO_CSTRING
#define _WTL_NO_WTYPES

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// doesn't work with STL
/*
#ifdef _DEBUG
	#pragma warning(disable:4985)
	#include <atldbgmem.h>
#endif
*/
#include <atlstr.h>
#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlcom.h>
#include <atlhost.h>
#include <atlwin.h>
#include <atlctl.h>
#include <atlmisc.h>
#include <atlcoll.h>
#include <atlimage.h>
#include <atlcomtime.h>

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>
#include <atlsplit.h>
#include <atlctrlx.h>
#include <atltheme.h>
#include "atlwfile.h"
#include "Thread.h"

#include <Winhttp.h>
#pragma comment(lib, "Winhttp.lib")

#include <msxml2.h>
#pragma comment(lib, "msxml2.lib")

// STL
#include <utility>
#include <regex>
namespace std {
	namespace tr1 {
		typedef basic_regex<TCHAR> tregex;
		typedef match_results<const TCHAR*> tcmatch;
	}
}

#ifdef _DEBUG
   #define DEBUG_CLIENTBLOCK   new( _CLIENT_BLOCK, __FILE__, __LINE__)
#else
   #define DEBUG_CLIENTBLOCK
#endif // _DEBUG

#define countof(x) (sizeof(x) / sizeof(x[0]))

// for SSL
/*
#define SECURITY_WIN32
#include <wincrypt.h>
#include <wintrust.h>
#include <schannel.h>
#include <security.h>
#include <sspi.h>
#pragma comment(lib, "crypt32.Lib")
#pragma comment(lib, "secur32.lib")
*/

#if defined _M_IX86
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
