#ifndef CLASSFACTORY_H
#define CLASSFACTORY_H

#pragma once
#include "pch.h"

class CContextMenuExt;

class CClassFactory final : public IClassFactory
{
    public:
        // IUnknown
        IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv) override;
        IFACEMETHODIMP_(ULONG) AddRef() override;
        IFACEMETHODIMP_(ULONG) Release() override;

        // IClassFactory
        IFACEMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject) override;
        IFACEMETHODIMP LockServer(BOOL fLock) override;

        CClassFactory();

    protected:
        ~CClassFactory();

    private:
        long m_cRef;
};

HRESULT CClassFactory_CreateInstance(REFCLSID rclsid, REFIID riid, LPVOID *ppv);

#endif //CLASSFACTORY_H