cmake_minimum_required(VERSION 3.31)
project(ContextMenuHandler)

set(CMAKE_CXX_STANDARD 23)

add_library(ContextMenuHandler SHARED
        ContextMenuHandler.rc
        dllmain.cpp
        ContextMenuHandler.def
        Guids.h
        ClassFactory.cpp
        ClassFactory.h
        ContextMenuExt.cpp
        ContextMenuExt.h
        Registerer.cpp
        Registerer.h
        Tools/ExtChanger.cpp
        Tools/ExtChanger.h
        Tools/UploadToTmpFiles.cpp
        Tools/UploadToTmpFiles.h
        pch.cpp
        pch.h
        resource.h
)




target_precompile_headers(ContextMenuHandler PRIVATE pch.h)

target_link_libraries(ContextMenuHandler PRIVATE
        shell32
        ole32
        oleaut32
        uuid
        comdlg32
        advapi32
        shlwapi
        Wininet
)

target_compile_definitions(ContextMenuHandler PRIVATE BUILDING_MY_DLL)

#set_target_properties(ContextMenuHandler PROPERTIES
#        LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/ContextMenuHandler.def"
#)


find_package(OpenCV REQUIRED)
target_link_libraries(ContextMenuHandler PRIVATE ${OpenCV_LIBS})
target_include_directories(ContextMenuHandler PRIVATE ${OpenCV_INCLUDE_DIRS})