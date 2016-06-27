# ===========================================
# Copyright KAPSARC. Open Source MIT License 
# ===========================================


project(smp)

cmake_minimum_required (VERSION 2.8)
cmake_policy(SET CMP0017 NEW) # prefer built-in modules

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

if (UNIX)
    set (ENABLE_EFFCPP true CACHE  BOOL "Check Effective C++ Guidelines")
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

set (CSVPARSER_DIR
    ${PROJECT_SOURCE_DIR}/../../KTAB/kmodel/csvparser/
    )

include_directories (
    ${KUTILS_INCLUDE_DIR}
    ${KMODEL_INCLUDE_DIR}
    ${KGRAPH_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/libsrc/
    ${PROJECT_SOURCE_DIR}/src/
    ${FLUID_DIR}
    ${FLTK_INCLUDE_DIR}
    ${CSVPARSER_DIR}
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
    ${CSVPARSER_DIR}/csv_parser.cpp
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

    #specifing path to find libraries
    if(WIN32)
        set(CMAKE_PREFIX_PATH "D:\\Qt\\Qt5.6.1\\5.6\\msvc2015_64\\lib\\cmake")
    elseif(UNIX)
        set(CMAKE_PREFIX_PATH "/home/ahowe/Qt5.6.1/5.6/gcc_64/lib/cmake")
    endif(WIN32)

    #adding qt libraries
    find_package(Qt5 REQUIRED COMPONENTS Widgets Core Sql PrintSupport)

    #manually adding files to the variables
    set (QtProjectLib_src ../../examples/smp/SMPQ/mainwindow.cpp
        ../../examples/smp/SMPQ/csv.cpp
        ../../examples/smp/SMPQ/database.cpp
        ../../examples/smp/SMPQ/qcustomplot.cpp
        ../../examples/smp/SMPQ/customgraph.cpp)
    set (QtProjectLib_hdr ../../examples/smp/SMPQ/mainwindow.h
        ../../examples/smp/SMPQ/csv.h
        ../../examples/smp/SMPQ/database.h
        ../../examples/smp/SMPQ/qcustomplot.h)
    set (QtProjectLib_ui  ../../examples/smp/SMPQ/mainwindow.ui)
    set (QtProjectBin_src ../../examples/smp/SMPQ/main.cpp)
    set (QtProjectRsc_qrc ../../examples/smp/SMPQ/dockwidgets.qrc)

    # qt5_wrap_cpp is used to create moc files of the cpp files
    qt5_wrap_cpp(QtProjectLib_hdr_moc ${QtProjectLib_hdr})
    # qt5_wrap_ui is used to create code from ui files
    qt5_wrap_ui(QtProjectLib_ui_uic ${QtProjectLib_ui})
    # qt5_wrd_resources is used to add resources from qrc file
    qt5_add_resources(QtProjectLib_qrc_rcc ${QtProjectRsc_qrc})

    #including source and binary directories
    include_directories (${PROJECT_SOURCE_DIR})
    include_directories (${PROJECT_BINARY_DIR})

    add_library (QtProjectLib STATIC
        ${QtProjectLib_src}# adding source files of source directory to the library
        ${QtProjectLib_hdr_moc}# adding the moc files generated using header files to the library
        ${QtProjectLib_qrc_rcc} # adding the rcc files generated using qrc files to the library
        ${QtProjectLib_ui_uic}) # adding the uic files generated using ui files to the library

    target_link_libraries (QtProjectLib Qt5::Core)
    target_link_libraries (QtProjectLib Qt5::Gui)
    target_link_libraries (QtProjectLib Qt5::Widgets)
    target_link_libraries (QtProjectLib Qt5::Sql)
    target_link_libraries (QtProjectLib Qt5::PrintSupport)

    add_executable(smpq ${QtProjectBin_src})

    #to link a library to the executable
    target_link_libraries (smpq QtProjectLib)

endif (ENABLE_QT_GUI)
#------------------------------------------------------------------

# show some useful status/debugging information
message (STATUS "Using PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR})
message (STATUS "KUTILS_INCLUDE_DIR: " ${KUTILS_INCLUDE_DIR})
message (STATUS "KMODEL_INCLUDE_DIR: " ${KMODEL_INCLUDE_DIR})

if (ENABLE_FLTK_GUI)
    message (STATUS "FLTK_INCLUDE_DIR: " ${FLTK_INCLUDE_DIR})
    message (STATUS "KGRAPH_INCLUDE_DIR: " ${KGRAPH_INCLUDE_DIR})
endif (ENABLE_FLTK_GUI)

# -------------------------------------------------
# As of early 2014, C++11 is still not the default
# for g++, so I have to provide it here.
# Also on Unix, the C++11 thread library relies on pthreads
if (UNIX OR MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++11 ")
    if (ENABLE_EFFCPP)
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weffc++ ")
    endif (ENABLE_EFFCPP)
endif(UNIX OR MINGW)

# ===========================================
# Copyright KAPSARC. Open Source MIT License 
# ===========================================
