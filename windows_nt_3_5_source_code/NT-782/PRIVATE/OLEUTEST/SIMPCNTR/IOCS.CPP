//**********************************************************************
// File name: IOCS.CPP
//
//      Implementation file for COleClientSite
//
// Functions:
//
//      See IOCS.H for class definition
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "iocs.h"
#include "ias.h"
#include "ioipf.h"
#include "ioips.h"
#include "app.h"
#include "site.h"
#include "doc.h"


//**********************************************************************
//
// COleClientSite::QueryInterface
//
// Purpose:
//
//      Used for interface negotiation
//
// Parameters:
//
//      REFIID riid         -   A reference to the interface that is
//                              being queried.
//
//      LPVOID FAR* ppvObj  -   An out parameter to return a pointer to
//                              the interface.
//
// Return Value:
//
//      S_OK                -   The interface is supported.
//      E_NOINTERFACE       -   The interface is not supported
//
// Function Calls:
//      Function                    Location
//
//      OutputDebugString           Windows API
//      CSimpleSite::QueryInterface SITE.CPP
//
//
//********************************************************************

STDMETHODIMP COleClientSite::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    OutputDebugString(TEXT("In IOCS::QueryInterface\r\n"));

    // delegate to the container Site
    return m_pSite->QueryInterface(riid, ppvObj);
}

//**********************************************************************
//
// COleClientSite::AddRef
//
// Purpose:
//
//      Increments the reference count of CSimpleSite. Since COleClientSite
//      is a nested class of CSimpleSite, we don't need to have a separate
//      reference count for COleClientSite. We can safely use the reference
//      count of CSimpleSite.
//
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of CSimpleSite
//
// Function Calls:
//      Function                    Location
//
//      OutputDebugString           Windows API
//      CSimpleSite::AddRef         SITE.CPP
//
//
//********************************************************************

STDMETHODIMP_(ULONG) COleClientSite::AddRef()
{
    OutputDebugString(TEXT("In IOCS::AddRef\r\n"));

    // delegate to the container Site
    return m_pSite->AddRef();
}


//**********************************************************************
//
// COleClientSite::Release
//
// Purpose:
//
//      Decrements the reference count of CSimpleSite. Since COleClientSite
//      is a nested class of CSimpleSite, we don't need to have a separate
//      reference count for COleClientSite. We can safely use the reference
//      count of CSimpleSite.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of CSimpleSite
//
// Function Calls:
//      Function                    Location
//
//      OutputDebugString           Windows API
//      CSimpleSite::Release        SITE.CPP
//
//
//********************************************************************

STDMETHODIMP_(ULONG) COleClientSite::Release()
{
    OutputDebugString(TEXT("In IOCS::Release\r\n"));

    // delegate to the container Site
    return m_pSite->Release();
}

//**********************************************************************
//
// COleClientSite::SaveObject
//
// Purpose:
//
//      Called by the object when it wants to be saved to persistant
//      storage
//
// Parameters:
//
//      None
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                            Location
//
//      OutputDebugString                   Windows API
//      IOleObject::QueryInterface          Object
//      IPersistStorage::SaveCompleted      Object
//      IPersistStorage::Release            Object
//      OleSave                             OLE API
//      GetScode                            OLE API
//      ResultFromScode                     OLE API
//
//
//********************************************************************

STDMETHODIMP COleClientSite::SaveObject()
{
    LPPERSISTSTORAGE lpPS;
    SCODE sc = E_FAIL;

    OutputDebugString(TEXT("In IOCS::SaveObject\r\n"));

    // get a pointer to IPersistStorage
    HRESULT hErr = m_pSite->m_lpOleObject->QueryInterface(IID_IPersistStorage,
                                                        (LPVOID FAR *)&lpPS);

    // save the object
    if (hErr == NOERROR)
    {
        sc = GetScode( OleSave(lpPS, m_pSite->m_lpObjStorage, TRUE) );
        lpPS->SaveCompleted(NULL);
        lpPS->Release();
    }

    return ResultFromScode(sc);
}

//**********************************************************************
//
// COleClientSite::GetMoniker
//
// Purpose:
//
//      Not Implemented
//
// Parameters:
//
//      DWORD  dwAssign         -   type of moniker to return
//      DWORD  dwWhichMoniker   -   which moniker to return
//
// Return Value:
//      E_NOTIMPL
//
// Function Calls:
//      Function                    Location
//
//      OutputDebugString           Windows API
//      ResultFromScode             OLE API
//
// Comments:
//
//      This function is not implemented because we don't support
//      linking.
//
//********************************************************************

STDMETHODIMP COleClientSite::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk)
{
    OutputDebugString(TEXT("In IOCS::GetMoniker\r\n"));

    // need to null the out pointer
    *ppmk = NULL;

    return ResultFromScode(E_NOTIMPL);
}

//**********************************************************************
//
// COleClientSite::GetContainer
//
// Purpose:
//
//      Not Implemented
//
// Parameters:
//
//      LPOLECONTAINER  FAR * ppContainer  -  where IOleContainer interface
//                                            pointer to be returned
//
// Return Value:
//
//      E_NOTIMPL
//
// Function Calls:
//      Function                    Location
//
//      OutputDebugString           Windows API
//
//
//********************************************************************

STDMETHODIMP COleClientSite::GetContainer(LPOLECONTAINER FAR* ppContainer)
{
    OutputDebugString(TEXT("In IOCS::GetContainer\r\n"));

    // NULL the out pointer
    *ppContainer = NULL;

    return ResultFromScode(E_NOTIMPL);
}

//**********************************************************************
//
// COleClientSite::ShowObject
//
// Purpose:
//
//      Not Implemented
//
// Parameters:
//
//      None
//
// Return Value:
//
//      NOERROR
//
// Function Calls:
//      Function                    Location
//
//      OutputDebugString           Windows API
//
// Comments:
//
//      This function is not implemented because we don't support
//      linking.
//
//********************************************************************

STDMETHODIMP COleClientSite::ShowObject()
{
    OutputDebugString(TEXT("In IOCS::ShowObject\r\n"));
    return NOERROR;
}

//**********************************************************************
//
// COleClientSite::OnShowWindow
//
// Purpose:
//
//      Object calls this method when it is opening/closing non-InPlace
//      Window
//
// Parameters:
//
//      BOOL fShow  - TRUE if Window is opening, FALSE if closing
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      OutputDebugString           Windows API
//      InvalidateRect              Windows API
//      BringWindowToTop            Windows API
//      SetFocus                    Windows API
//      ResultFromScode             OLE API
//
//
//********************************************************************

STDMETHODIMP COleClientSite::OnShowWindow(BOOL fShow)
{
    OutputDebugString(TEXT("In IOCS::OnShowWindow\r\n"));
    m_pSite->m_fObjectOpen = fShow;
    InvalidateRect(m_pSite->m_lpDoc->m_hDocWnd, NULL, TRUE);

    // if object window is closing, then bring container window to top
    if (! fShow)
    {
        BringWindowToTop(m_pSite->m_lpDoc->m_hDocWnd);
        SetFocus(m_pSite->m_lpDoc->m_hDocWnd);
    }
    return ResultFromScode(S_OK);
}

//**********************************************************************
//
// COleClientSite::RequestNewObjectLayout
//
// Purpose:
//
//      Not Implemented
//
// Parameters:
//
//      None
//
// Return Value:
//
//      E_NOTIMPL
//
// Function Calls:
//      Function                    Location
//
//      OutputDebugString           Windows API
//
//
//********************************************************************

STDMETHODIMP COleClientSite::RequestNewObjectLayout()
{
    OutputDebugString(TEXT("In IOCS::RequestNewObjectLayout\r\n"));
    return ResultFromScode(E_NOTIMPL);
}
