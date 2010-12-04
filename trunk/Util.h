#pragma once

namespace Util
{
	void CreateConsole();
	void Print(const char* s);
	void Print(const wchar_t* s);
	CString FormatSize(__int64 size);
	CString FormatSpeed(__int64 speed);
	CString FormatTimeSpan(__int64 time);
	__int64 ParseSize(const CString& s);
	__int64 ParseTimeSpan(const CString& s);
	CString SanitizeFilename(const CString& fn);
	void DeleteDirectory(const CString& path);
	void RegisterAssoc(HINSTANCE hInstance);
	void UnregisterAssoc();
	CString BrowseForFolder(HWND hwndParent, const TCHAR* title, const TCHAR* initialDir);
	CWindow GetMainWindow();
	void SetMainWindow(CWindow wnd);
	CString GetErrorMessage(int error);
	int TestCreateDirectory(const CString& path); // return: system error codes (ERROR_SUCCESS, ...)
};

class CToolBarImageList
{
public:
	~CToolBarImageList();

	bool Load(const CString& path, int cx, int cy);
	bool LoadFromResource(ATL::_U_STRINGorID bitmap, int cx, int cy);

	bool Load(CImage &image, int cx, int cy);
	void Set(HWND hwndToolBar);

protected:
	CImageList ilNormal, ilDisabled;
};