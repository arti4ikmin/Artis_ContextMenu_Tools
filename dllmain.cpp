// This is the main file, strangely Ive read out that usually the lib mains contain only "standart" code and its unusual to stuff it up with your own

#include <initguid.h>
#define INITGUID
#include "Guids.h"
// DO NOT REMOVE GUIDSH
#include "pch.h"
#include "ClassFactory.h"
#include "Registerer.h"

HINSTANCE g_hInstDll = nullptr;
long g_cRefModule = 0; // global reference count

BOOL APIENTRY DllMain(const HMODULE hModule, const DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            g_hInstDll = hModule;
            DisableThreadLibraryCalls(hModule); // optimization
            break;
        case DLL_THREAD_ATTACH:
        
        case DLL_THREAD_DETACH:
        
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}


STDAPI DllCanUnloadNow(void)
{
    return g_cRefModule == 0 ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return CClassFactory_CreateInstance(rclsid, riid, ppv);
}

STDAPI DllRegisterServer(void)
{
    return RegisterContextMenuHandler();
}

STDAPI DllUnregisterServer(void)
{
    return UnregisterContextMenuHandler();
}
