#ifndef REGISTERER_H
#define REGISTERER_H


#ifdef BUILDING_MY_DLL
    #define DLL_API __declspec(dllexport)
#else
    #define DLL_API __declspec(dllimport)
#endif

DLL_API long RegisterContextMenuHandler();
DLL_API long UnregisterContextMenuHandler();

#endif //REGISTERER_H
