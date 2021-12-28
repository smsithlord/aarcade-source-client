#ifndef DropTargetH
#define DropTargetH
#include <ole2.h>
#include <shlobj.h>

#define WM_OLEDROP WM_USER + 1
//---------------------------------------------------------------------------

class TDropTarget : public IDropTarget
{
private:
	unsigned long FReferences;
	bool m_bIsDragging;
	HWND FFormHandle;

	// helper function
	void __fastcall HandleDrop(HDROP HDrop);

protected:
	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppvObj);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IDropTarget methods
	STDMETHOD(DragEnter)(LPDATAOBJECT pDataObj, DWORD grfKeyState,
		POINTL pt, LPDWORD pdwEffect);
	STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
	STDMETHOD(DragLeave)();
	STDMETHOD(Drop)(LPDATAOBJECT pDataObj, DWORD grfKeyState,
		POINTL pt, LPDWORD pdwEffect);

public:
	__fastcall TDropTarget(HWND HForm);
	__fastcall TDropTarget();
	__fastcall ~TDropTarget();

};
//---------------------------------------------------------------------------
#endif


// file: MyEdDropTarget.h
//
//#include <windows.h>
//#include <ole2.h>

/*
#include <afxole.h>
class CMyEdDropTarget : public COleDropTarget
{
public:
    CMyEdDropTarget::CMyEdDropTarget() { // CTOR
        OleInitialize(0);
        m_CF_URLA= RegisterClipboardFormat( _TEXT("UniformResourceLocator") );
        m_CF_URLW= RegisterClipboardFormat( _TEXT("UniformResourceLocatorW") );
    }
    virtual DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject,
                DWORD dwKeyState, CPoint point) 
    {
        STGMEDIUM rSM;
        BOOL fRet= pDataObject->GetData( m_CF_URLA, &rSM ); 
        if ( fRet ) {
            return( DROPEFFECT_LINK ); // "Drop OK"
        }
        return( DROPEFFECT_NONE ); //else, show the "Don't Drop" cursor
    };
    virtual BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject,
                DROPEFFECT dropEffect, CPoint point) 
    {
        STGMEDIUM rSM;
        BOOL fRet= pDataObject->GetData( m_CF_URLA, &rSM ); 

        char* p= (char*)GlobalLock(rSM.hGlobal); 
       // CStringW sw( p );    // convert to UNICODE 
        GlobalUnlock( rSM.hGlobal );

		LPCSTR ptr = p;

        CEdit* pEd= (CEdit*)pWnd;
        pEd->SetSel(0,-1);
		pEd->ReplaceSel(ptr);

        return( 1 ); // success
    };
    UINT m_CF_URLA;
    UINT m_CF_URLW;
};
*/