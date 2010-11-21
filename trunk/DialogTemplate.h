#pragma once

class CDialogTemplate
{
// Constructors
public:
	/* explicit */ CDialogTemplate(const DLGTEMPLATE* pTemplate = NULL);
	explicit CDialogTemplate(HGLOBAL hGlobal);

// Attributes
	BOOL HasFont() const;
	BOOL SetFont(LPCTSTR lpFaceName, WORD nFontSize);
	BOOL SetSystemFont(WORD nFontSize = 0);
	BOOL GetFont(CString& strFaceName, WORD& nFontSize) const;
	void GetSizeInDialogUnits(SIZE* pSize) const;
	void GetSizeInPixels(SIZE* pSize) const;

	static BOOL __cdecl GetFont(const DLGTEMPLATE* pTemplate,
		CString& strFaceName, WORD& nFontSize);

// Operations
	BOOL Load(LPCTSTR lpDialogTemplateID);
	HGLOBAL Detach();

// Implementation
public:
	~CDialogTemplate();

	HGLOBAL m_hTemplate;
	DWORD m_dwTemplateSize;
	BOOL m_bSystemFont;

protected:
	static BYTE* __cdecl GetFontSizeField(const DLGTEMPLATE* pTemplate);
	static UINT __cdecl GetTemplateSize(const DLGTEMPLATE* pTemplate);
	BOOL SetTemplate(const DLGTEMPLATE* pTemplate, UINT cb);
};
