# ===========================================
# Copyright KAPSARC. Open Source MIT License 
# ===========================================


project(smp)

cmake_minimum_required (VERSION 3.0)
cmake_policy(VERSION 3.0)
cmake_policy(SET CMP0017 NEW) # prefer built-in modules
cmake_policy(SET CMP0020 NEW) # Automatically link Qt executables to qtmain


set (LOCAL_MODULE_DIR
    ${PROJECT_SOURCE_DIR}/../../KTAB/cmakemodules)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${LOCAL_MODULE_DIR}
    )

set(CMAKE_BUILD_TYPE Debug)

set(LIBRARY_OUTPUT_PATH      ${PROJECT_SOURCE_DIR}/)
set(EXECUTABLE_OUTPUT_PATH   ${PROJECT_SOURCE_DIR}/)

set (ENABLE_FLTK_GUI true CACHE  BOOL "Build app with FLTK GUI")
set (ENABLE_QT_GUI true CACHE  BOOL "Build app with QT GUI")
set (ENABLE_COPY_QT_LIBS false CACHE  BOOL "Copy Qt LIBS after build")

if (UNIX)
    set (ENABLE_EFFCPP false CACHE  BOOL "Check Effective C++ Guidelines")
    set (ENABLE_EFENCE false CACHE  BOOL "Use Electric Fence memory debugger")
endif(UNIX)

# -------------------------------------------------

if (ENABLE_EFENCE)
    find_package(efence)
    if (NOT EFENCE_FOUND)
        message(FATAL_ERROR "Could not find Electric Fence memory debugger")
    endif (NOT EFENCE_FOUND)
endif (ENABLE_EFENCE)

# --------------------------------------------------------
find_package(Sqlite)
if (NOT SQLITE_FOUND)
    message(FATAL_ERROR "Could not find SQLite")
endif (NOT SQLITE_FOUND)

# --------------------------------------------------------
# See this page for useful hints: https://cmake.org/Wiki/CMakeForFLTK

set (SMP_XLIBS  )

# --------------------------------------------------------
# See "Findkutils.cmake" in cmakemodules to figure
# out where it looks and what variables it sets
find_package(kutils)
if(NOT KUTILS_FOUND)
    message(FATAL_ERROR "Could not find kutils")
endif(NOT KUTILS_FOUND)


# Ditto for "Findkmodel.cmake"
find_package(kmodel)
if(NOT KMODEL_FOUND)
    message(FATAL_ERROR "Could not find kmodel")
endif(NOT KMODEL_FOUND)


if (ENABLE_FLTK_GUI)
    
    if(UNIX)
        find_package(X11)
        if (NOT X11_FOUND)
            message (FATAL_ERROR "Couldn't find X11: required for building smpg")
        endif (NOT X11_FOUND)
        set (SMP_XLIBS
            #           Xaw Xmu Xpm Xt Xft X11 Xinerama dl fontconfig
            # Xext Xft fontconfig Xinerama dl m X11
            Xext Xft fontconfig dl m X11  Xfixes
            # Xft dl m X11
            )
    endif (UNIX)

    include (${LOCAL_MODULE_DIR}/FLTKHelper.cmake)
    find_package(FLTK REQUIRED)
    if (NOT FLTK_FOUND)
        message (FATAL_ERROR "Could not find fltk: required for building smpg")
    endif (NOT FLTK_FOUND)

    set (FLUID_DIR
        fluid)


    # Ditto for "Findkgraph.cmake"
    find_package(kgraph)
    if(NOT KGRAPH_FOUND)
        message(FATAL_ERROR "Could not find kgraph")
    endif(NOT KGRAPH_FOUND)
endif (ENABLE_FLTK_GUI)

# -------------------------------------------------
# files to be built into a library go in libsrc/
# any application or testing code goes in src
# -------------------------------------------------

set (CSVPARSER_PATHS
    ${PROJECT_SOURCE_DIR}/../../csv-parser/
    )

find_path(CSVP_DIR csv_parser.hpp 
    PATHS ${CSVPARSER_PATHS}
    )

if(CSVP_DIR)
    message(STATUS "Found csv-parser headers: ${CSVP_DIR}")
endif(CSVP_DIR)

if(NOT CSVP_DIR)
    message(FATAL "Could not find csv-parser headers")
endif(NOT CSVP_DIR)


# -------------------------------------------------    

include_directories (
    ${KUTILS_INCLUDE_DIR}
    ${KMODEL_INCLUDE_DIR}
    ${KGRAPH_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/libsrc/
    ${PROJECT_SOURCE_DIR}/src/
    ${FLUID_DIR}
    ${FLTK_INCLUDE_DIR}
    ${CSVP_DIR}
    ${SQLITE_INCLUDE_DIR}
    )

# note that we do not put the csv_parser into smp lib.
set(SMPLIB_SRCS
    ${PROJECT_SOURCE_DIR}/libsrc/smp.cpp
    ${PROJECT_SOURCE_DIR}/libsrc/smpsql.cpp
    )

add_library(smp STATIC ${SMPLIB_SRCS})

# -------------------------------------------------
# Under Linux, default start of DESTINATION path is /usr/local/ 
# so "ktab/lib" denotes destination /usr/local/ktab/lib
# Under Windows, default start of DESTINATION path is "C:\Program Files\kutils\"
# so "ktab/lib" denotes destination C:\Program Files\kutils\ktab\lib

set (SMP_INSTALL_DIR)
if (WIN32)
    set (SMP_INSTALL_DIR
        c:/local/ktab
        )
endif(WIN32)
if (UNIX)
    set (SMP_INSTALL_DIR
        /usr/local/ktab
        )
endif(UNIX)

install(
    TARGETS
    smp
    DESTINATION
    ${SMP_INSTALL_DIR}/lib)

install(
    FILES
    libsrc/smp.h
    DESTINATION
    ${SMP_INSTALL_DIR}/include)

# -------------------------------------------------
if (ENABLE_FLTK_GUI)
    # define a little library of smp GUI stuff (if desired)

    set (SMPGUI_FLS
        ${FLUID_DIR}/mainw.fl
        )

    message (STATUS "Use these SMPGUI_FLS: ${SMPGUI_FLS}")

    fltk_wrap_ui (smpGUI ${SMPGUI_FLS})
    add_library (smpGUI STATIC ${smpGUI_FLTK_UI_SRCS})

    # --------------------------------------------------------
    set (SMPG_SRCS
        ${PROJECT_SOURCE_DIR}/src/guimain.cpp
        )
    set (SMPG_LIBS
        ${FLTK_LIBRARIES}
        ${KGRAPH_LIBRARIES}
        smpGUI
        ${SMP_XLIBS}
        )

    add_executable (smpg ${SMPG_SRCS})
    target_link_libraries(smpg
        smp
        ${SMPG_LIBS}
        ${SQLITE_LIBRARIES}
        ${EFENCE_LIBRARIES}
        )

    # suppress the console window under Windows
    if (WIN32)
        if (MSVC)
            set_target_properties(smpg PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
            set_target_properties(smpg PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
            message (STATUS "Using /SUBSYSTEM:WINDOWS")
        endif(MSVC)
    endif(WIN32)
endif (ENABLE_FLTK_GUI)
# -------------------------------------------------

add_executable (smpc
    src/demosmp.cpp
    ${CSVP_DIR}/csv_parser.cpp
    )

target_link_libraries (smpc
    smp
    ${KMODEL_LIBRARY}
    ${KUTILS_LIBRARY}
    ${SQLITE_LIBRARIES}
    ${EFENCE_LIBRARIES}
    )

# -------------------------------------------------


#--------------------------------------------------
#smpq qt based application

if(ENABLE_QT_GUI)

    find_package(smpq)
    if(NOT SMPQ_FOUND)
        message(FATAL_ERROR "Could not find SMPQ")
    endif(NOT SMPQ_FOUND)

    set(CMAKE_PREFIX_PATH ${PREFIX_PATH})
    message(STATUS "CMAKE_PREFIX_PATH" ${CMAKE_PREFIX_PATH})

    find_package(Qt5 REQUIRED COMPONENTS Widgets Core Sql PrintSupport)

    qt5_wrap_cpp(QtProjectLib_hdr_moc ${QtProjectLib_hdr})
    qt5_wrap_ui(QtProjectLib_ui_uic ${QtProjectLib_ui})
    qt5_add_resources(QtProjectLib_qrc_rcc ${QtProjectRsc_qrc})

    include_directories (${PROJECT_SOURCE_DIR})
    include_directories (${PROJECT_BINARY_DIR})

    add_library (QtProjectLib STATIC ${QtProjectLib_src} ${QtProjectLib_hdr_moc} ${QtProjectLib_qrc_rcc}
        ${QtProjectLib_ui_uic})

    target_link_libraries (QtProjectLib Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Sql Qt5::PrintSupport)

    # WIN32 to suppress the console window under Windows
    add_executable(smpq WIN32 ${QtProjectBin_src})

    target_link_libraries (smpq QtProjectLib)

    if(ENABLE_COPY_QT_LIBS AND WIN32)
        add_custom_command(TARGET smpq POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${EXECUTABLE_OUTPUT_PATH}smpq.exe ${EXECUTABLE_OUTPUT_PATH}smpq_bin)
        find_package(qtdlls)
    endif(ENABLE_COPY_QT_LIBS AND WIN32)

endif (ENABLE_QT_GUI)

#------------------------------------------------------------------

# show some useful status/debugging information
message (STATUS "Using PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR})
message (STATUS "KUTILS_INCLUDE_DIR: " ${KUTILS_INCLUDE_DIR})
message (STATUS "KMODEL_INCLUDE_DIR: " ${KMODEL_INCLUDE_DIR})
message (STATUS "CSVP_DIR: " ${CSVP_DIR})
message (STATUS "QCUSTOMPLOT_DIR: " ${QCUSTOMPLOT_DIR})

if (ENABLE_FLTK_GUI)
    message (STATUS "FLTK_INCLUDE_DIR: " ${FLTK_INCLUDE_DIR})
    message (STATUS "KGRAPH_INCLUDE_DIR: " ${KGRAPH_INCLUDE_DIR})
endif (ENABLE_FLTK_GUI)



# -------------------------------------------------
# As of early 2014, C++11 is still not the default
# for g++, so I have to provide it here.
# Also on Unix, the C++14 thread library relies on pthreads
if (UNIX OR MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++14 ")
    if (ENABLE_EFFCPP)
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weffc++ ")
    endif (ENABLE_EFFCPP)
endif(UNIX OR MINGW)

# ===========================================
# Copyright KAPSARC. Open Source MIT License 
# ===========================================
