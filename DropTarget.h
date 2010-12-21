#pragma once

// see http://www.koders.com/cpp/fidBE346D5A55C2F375F9EDE33E01F8651ABEDCC36A.aspx
// and http://blogs.msdn.com/b/oldnewthing/archive/2010/05/03/10006065.aspx

#include <intshcut.h>
#include <shlobj.h>

// Drop Target can receive URLs (from browser or .URL shortcut files) or Files (simulating WS_EX_ACCEPTFILES/WM_DROPFILES)
// T is the class name of the object that will be notified upon dropped files/URLs

// T must have:
// 	 void OnDropFile(LPCTSTR szFile);
//	 void OnDropURL(LPCTSTR szURL);

// Example:
//  T::OnCreate or T::OnInitDialog
//	  CComObject<CDropTarget<T> >* pDropTarget;
//	  CComObject<CDropTarget<T> >::CreateInstance(&pDropTarget);
//	  pDropTarget->Register(*this, this);
//  T::OnDestroy
// 	  ::RevokeDragDrop(*this);

template <class T>
class CDropTarget : public CComObjectRootEx<CComSingleThreadModel>, public IDropTarget
{
public:
	// register drag/drop for window hWnd and notification target pTarget
	void Register(HWND hWnd, T* pTarget)
	{
		m_pTarget = pTarget;
		::RegisterDragDrop(hWnd, this);
	}

protected:
	CDropTarget()
	{
		m_pTarget = NULL;
	}

	BEGIN_COM_MAP(CDropTarget<T>)
		COM_INTERFACE_ENTRY(IDropTarget)
	END_COM_MAP()

	T* m_pTarget;

	// Helper method to extract a URL from an internet shortcut file
	HRESULT GetURLFromFile(const TCHAR *pszFile, CString &szURL)
	{
		USES_CONVERSION;
		CComQIPtr<IUniformResourceLocator> spUrl;

		HRESULT hr = spUrl.CoCreateInstance(CLSID_InternetShortcut);
		if(FAILED(hr))
			return E_FAIL;

		CComQIPtr<IPersistFile> spFile = spUrl;
		if(!spFile)
			return E_FAIL;

		// Initialize the URL object from the filename
		LPWSTR lpTemp = NULL;
		if(FAILED(spFile->Load(T2OLE((LPTSTR)pszFile), STGM_READ)) || FAILED(spUrl->GetURL(&lpTemp)))
			return E_FAIL;

		if(!lpTemp)
			return E_FAIL;

		// Free the memory
		CComQIPtr<IMalloc> spMalloc;
		if(FAILED(SHGetMalloc(&spMalloc)))
			return E_FAIL;

		// Copy the URL & cleanup
		szURL = W2T(lpTemp);
		spMalloc->Free(lpTemp);

		return S_OK;
	}

	// Data object currently being dragged
	CComQIPtr<IDataObject> m_spDataObject;

	// IDropTarget
	virtual HRESULT STDMETHODCALLTYPE DragEnter(/* [unique][in] */ IDataObject __RPC_FAR *pDataObj, /* [in] */ DWORD grfKeyState, /* [in] */ POINTL pt, /* [out][in] */ DWORD __RPC_FAR *pdwEffect)
	{
		if(pdwEffect == NULL || pDataObj == NULL) {
			ASSERT(FALSE);
			return E_INVALIDARG;
		}

		if(m_spDataObject != NULL) {
			ASSERT(FALSE);
			return E_UNEXPECTED;
		}

		FORMATETC formatetc;
		memset(&formatetc, 0, sizeof(formatetc));
		formatetc.dwAspect = DVASPECT_CONTENT;
		formatetc.lindex = -1;
		formatetc.tymed = TYMED_HGLOBAL;

		// Test for HDROP (WM_DROPFILES compatible multi-file-drop)
		formatetc.cfFormat = CF_HDROP;
		if(pDataObj->QueryGetData(&formatetc) == S_OK) {
			m_spDataObject = pDataObj;
			*pdwEffect = DROPEFFECT_LINK;
			return S_OK;
		}

		// Test if the data object contains a text URL format
		formatetc.cfFormat = g_cfURL;
		if(pDataObj->QueryGetData(&formatetc) == S_OK) {
			m_spDataObject = pDataObj;
			*pdwEffect = DROPEFFECT_LINK;
			return S_OK;
		}

		// If we got here, then the format is not supported
		*pdwEffect = DROPEFFECT_NONE;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE DragOver(/* [in] */ DWORD grfKeyState, /* [in] */ POINTL pt, /* [out][in] */ DWORD __RPC_FAR *pdwEffect)
	{
		if(pdwEffect == NULL) {
			ASSERT(FALSE);
			return E_INVALIDARG;
		}
		*pdwEffect = m_spDataObject ? DROPEFFECT_LINK : DROPEFFECT_NONE;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE DragLeave(void)
	{
		if(m_spDataObject)
			m_spDataObject.Release();

		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Drop(/* [unique][in] */ IDataObject __RPC_FAR *pDataObj, /* [in] */ DWORD grfKeyState, /* [in] */ POINTL pt, /* [out][in] */ DWORD __RPC_FAR *pdwEffect)
	{
		CString sURL;

		if(pdwEffect == NULL) {
			ASSERT(FALSE);
			return E_INVALIDARG;
		}
		if(m_spDataObject == NULL) {
			*pdwEffect = DROPEFFECT_NONE;
			return S_OK;
		}

		*pdwEffect = DROPEFFECT_LINK;

		// Get the URL from the data object
		FORMATETC formatetc;
		STGMEDIUM stgmedium;
		memset(&formatetc, 0, sizeof(formatetc));
		memset(&stgmedium, 0, sizeof(stgmedium));

		formatetc.dwAspect = DVASPECT_CONTENT;
		formatetc.lindex = -1;
		formatetc.tymed = TYMED_HGLOBAL;

		// Does the data object contain a URL?
		formatetc.cfFormat = g_cfURL;
		if (m_spDataObject->GetData(&formatetc, &stgmedium) == S_OK) {
			ASSERT(stgmedium.tymed == TYMED_HGLOBAL);
			ASSERT(stgmedium.hGlobal);
			char *pszURL = (char *)GlobalLock(stgmedium.hGlobal);
			//TRACE("URL \"%s\" dragged over control\n", pszURL);
			sURL = CString(pszURL);
			if(m_pTarget)
				m_pTarget->OnDropURL(sURL);
			GlobalUnlock(stgmedium.hGlobal);
		} else {
			// Is it dropped files=
			formatetc.cfFormat = CF_HDROP;
			if (m_spDataObject->GetData(&formatetc, &stgmedium) == S_OK) {
				ASSERT(stgmedium.tymed == TYMED_HGLOBAL);
				ASSERT(stgmedium.hGlobal);
				HDROP hdrop = (HDROP)(stgmedium.hGlobal);
				UINT cFiles = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
				for (UINT i = 0; i < cFiles; i++) {
					TCHAR szFile[MAX_PATH];
					UINT cch = DragQueryFile(hdrop, i, szFile, MAX_PATH);
					if (cch > 0 && cch < MAX_PATH) {
						//TRACE(_T("HDROP/File \"%s\" dragged over control\n"), szFile);
						if(SUCCEEDED(GetURLFromFile(szFile, sURL))) {
							if(m_pTarget)
								m_pTarget->OnDropURL(sURL);
						} else {
							if(m_pTarget)
								m_pTarget->OnDropFile(szFile);
						}
					}
				}
			}
		}

		ReleaseStgMedium(&stgmedium);
		m_spDataObject.Release();

		return S_OK;
	}

protected:
	static const UINT g_cfURL;
};

template <class T>
const UINT CDropTarget<T>::g_cfURL = RegisterClipboardFormat(CFSTR_SHELLURL);
