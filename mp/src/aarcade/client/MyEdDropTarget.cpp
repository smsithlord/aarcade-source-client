#include "cbase.h"

//in source...
#include "MyEdDropTarget.h"

// memdbgon must be the last include file in a .cpp file!!!
//#include "tier0/memdbgon.h"
//---------------------------------------------------------------------------
__fastcall TDropTarget::TDropTarget(HWND HForm)
        : IDropTarget()
{
    FFormHandle = HForm;  // handle to your Form passed in the ctor
    FReferences = 1;
	m_bIsDragging = false;
}
//---------------------------------------------------------------------------
__fastcall TDropTarget::TDropTarget()
	: IDropTarget()
{
	FReferences = 1;
	m_bIsDragging = false;
}
//---------------------------------------------------------------------------
__fastcall TDropTarget::~TDropTarget()
{
}
//---------------------------------------------------------------------------

// helper routine to notify Form of drop on target
void __fastcall TDropTarget::HandleDrop(HDROP HDrop)
{
    SendMessage(FFormHandle, WM_OLEDROP, (WPARAM)HDrop, 0);
}
//---------------------------------------------------------------------------

#include "c_anarchymanager.h"

// IUnknown Interface has three member functions:
// QueryInterface, AddRef, and Release.

STDMETHODIMP TDropTarget::QueryInterface(REFIID iid, void FAR* FAR* ppv)
{
   // tell other objects about our capabilities
   if (iid == IID_IUnknown || iid == IID_IDropTarget)
   {
       *ppv = this;
       AddRef();
       return NOERROR;
   }
   *ppv = NULL;
   return ResultFromScode(E_NOINTERFACE);
}
//---------------------------------------------------------------------------

STDMETHODIMP_(ULONG) TDropTarget::AddRef()
{
   return ++FReferences;
}
//---------------------------------------------------------------------------

STDMETHODIMP_(ULONG) TDropTarget::Release()
{
   if (--FReferences == 0)
   {
       delete this;
       return 0;
   }
   return FReferences;
}
//---------------------------------------------------------------------------

// IDropTarget Interface handles the Drag and Drop
// implementation

// Drag Enter is called first
STDMETHODIMP TDropTarget::DragEnter(LPDATAOBJECT pDataObj, DWORD grfKeyState,
    POINTL pt, LPDWORD pdwEffect)
{
	/*
    FORMATETC fmtetc;

    fmtetc.cfFormat = CF_HDROP;
    fmtetc.ptd      = NULL;
    fmtetc.dwAspect = DVASPECT_CONTENT;
    fmtetc.lindex   = -1;
    fmtetc.tymed    = TYMED_HGLOBAL;

   // does the drag source provide CF_HDROP,
    // which is the only format we accept
   if (pDataObj->QueryGetData(&fmtetc) == NOERROR)
        FAcceptFormat = true;
   else FAcceptFormat = false;https://multistre.am/oke_doke/pux1g/layout3/
   */
	//FAcceptFormat = false;

	if (m_bIsDragging)
		DevMsg("WARNING: Already thought a drag was happening!\n");

	m_bIsDragging = g_pAnarchyManager->DragEnter();

	return (m_bIsDragging) ? NOERROR : DROPEFFECT_NONE;
}
//---------------------------------------------------------------------------

// implement visual feedback if required
STDMETHODIMP TDropTarget::DragOver(DWORD grfKeyState, POINTL pt, 
    LPDWORD pdwEffect)
{
	if (m_bIsDragging)
	{
		float flX = (pt.x * 1.0f) / ScreenWidth();
		float flY = (pt.y * 1.0f) / ScreenHeight();
		g_pAnarchyManager->GetInputManager()->MouseMove(flX, flY);
		return NOERROR;// DROPEFFECT_LINK;
	}

	return DROPEFFECT_NONE;
}
//---------------------------------------------------------------------------

// remove visual feedback
STDMETHODIMP TDropTarget::DragLeave()
{
	if (m_bIsDragging)
		g_pAnarchyManager->DragLeave();

	m_bIsDragging = false;

   return NOERROR;
}
//---------------------------------------------------------------------------

// source has sent the DRAGDROP_DROP message indicating
// a drop has a occurred
STDMETHODIMP TDropTarget::Drop(LPDATAOBJECT pDataObj, DWORD grfKeyState,
    POINTL pt, LPDWORD pdwEffect)
{
	if ( !m_bIsDragging )
		return NOERROR;
	
	bool bSuccess = false;

	// First, try to get a list of files...
	FORMATETC files_fmtetc;
	files_fmtetc.cfFormat = CF_HDROP;
	files_fmtetc.ptd = NULL;
	files_fmtetc.dwAspect = DVASPECT_CONTENT;
	files_fmtetc.lindex = -1;
	files_fmtetc.tymed = TYMED_HGLOBAL;

	if (pDataObj->QueryGetData(&files_fmtetc) == NOERROR)
	{
		STGMEDIUM files_medium;
		HRESULT files_hr = pDataObj->GetData(&files_fmtetc, &files_medium);

		if (!FAILED(files_hr))
		{
			// grab a pointer to the data
			HGLOBAL HFiles = files_medium.hGlobal;
			HDROP HDrop = (HDROP)GlobalLock(HFiles);

			// call the helper routine which will notify the Form
			// of the drop
			// HandleDrop(HDrop);
			g_pAnarchyManager->HandleDragDrop(HDrop);

			// release the pointer to the memory
			GlobalUnlock(HFiles);
			ReleaseStgMedium(&files_medium);

			bSuccess = true;
		}
	}

	if (!bSuccess)
	{
		FORMATETC text_fmtetc;
		text_fmtetc.cfFormat = CF_TEXT;
		text_fmtetc.ptd = NULL;
		text_fmtetc.dwAspect = DVASPECT_CONTENT;
		text_fmtetc.lindex = -1;
		text_fmtetc.tymed = TYMED_HGLOBAL;

		if (pDataObj->QueryGetData(&text_fmtetc) == NOERROR)
		{
			// user has dropped on us -- get the CF_HDROP data from drag source
			STGMEDIUM text_medium;
			HRESULT text_hr = pDataObj->GetData(&text_fmtetc, &text_medium);

			if (!FAILED(text_hr))
			{
				// Get handle of clipboard object for ANSI text
				HANDLE hData = text_medium.hGlobal;
				if (hData != nullptr)
				{
					// Lock the handle to get the actual text pointer
					char * pszText = static_cast<char*>(GlobalLock(hData));
					if (pszText != nullptr)
					{
						// Save text in a string class instance
						std::string text(pszText);

						g_pAnarchyManager->HandleTextDragDrop(text);

						bSuccess = true;
					}
				}

				// Release the lock
				GlobalUnlock(hData);

				ReleaseStgMedium(&text_medium);
			}
		}
	}

	if (!bSuccess)
		*pdwEffect = DROPEFFECT_NONE;
	
	HWND hwnd = g_pAnarchyManager->GetHWND();
	if (g_pAnarchyManager->MapName() && hwnd)
	{
		SetActiveWindow(hwnd);
		SetForegroundWindow(hwnd);
	}
		//BringWindowToTop(g_pAnarchyManager->GetHWND());

   return NOERROR;
}
//---------------------------------------------------------------------------