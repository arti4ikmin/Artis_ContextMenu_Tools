#include "pch.h"
#include "ClassFactory.h"
#include "Guids.h"
#include "ContextMenuExt.h"

extern long g_cRefModule;

CClassFactory::CClassFactory() : m_cRef(1)
{
    InterlockedIncrement(&g_cRefModule);
}

CClassFactory::~CClassFactory()
{
    InterlockedDecrement(&g_cRefModule);
}

// IUnknown methods (very painful)
IFACEMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CClassFactory, IClassFactory),
        { nullptr }
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

IFACEMETHODIMP_(ULONG) CClassFactory::Release()
{
    const long cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

// IClassFactory methods
IFACEMETHODIMP CClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
    if (ppvObject == nullptr)
    {
        return E_POINTER;
    }
    *ppvObject = nullptr;

    if (pUnkOuter != nullptr)
    {
        return CLASS_E_NOAGGREGATION;
    }

    // instance of context menu extension obj (used to be     CContextMenuExt *pExt , clion changed to:       (revert if not working))
    auto* pExt = new(std::nothrow) CContextMenuExt();
    if (pExt == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    const HRESULT hr = pExt->QueryInterface(riid, ppvObject);
    pExt->Release(); // QueryInterface AddRef

    return hr;
}

IFACEMETHODIMP CClassFactory::LockServer(const BOOL fLock)
{
    if (fLock)
    {
        InterlockedIncrement(&g_cRefModule);
    }
    else
    {
        InterlockedDecrement(&g_cRefModule);
    }
    return S_OK;
}

// DllGetClassObject will call to get instance of ClassFactory
HRESULT CClassFactory_CreateInstance(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if (ppv == nullptr)
    {
        return E_POINTER;
    }
    *ppv = nullptr;

    //
    if (!IsEqualCLSID(rclsid, CLSID_ContextMenuHandler))
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    // --//-- (l. 57)
    auto *pFactory = new (std::nothrow) CClassFactory();
    if (pFactory == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    const HRESULT hr = pFactory->QueryInterface(riid, ppv);
    pFactory->Release(); // QueryInterface AddRef

    return hr;
}