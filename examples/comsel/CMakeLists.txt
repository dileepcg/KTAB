# ===========================================
# Copyright KAPSARC. Open Source MIT License 
# ===========================================

project(comsel)

cmake_minimum_required (VERSION 3.0)
cmake_policy(VERSION 3.0)

set (LOCAL_MODULE_DIR
  ${PROJECT_SOURCE_DIR}/../../KTAB/cmakemodules)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${LOCAL_MODULE_DIR}
  )


set(CMAKE_BUILD_TYPE Debug)

set(LIBRARY_OUTPUT_PATH      ${PROJECT_SOURCE_DIR}/)
set(EXECUTABLE_OUTPUT_PATH   ${PROJECT_SOURCE_DIR}/)

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

# -------------------------------------------------
 
find_package(TinyXML2)
if (NOT TINYXML2_FOUND)
  message(FATAL_ERROR "Could not find TinyXML2")
endif (NOT TINYXML2_FOUND) 

# --------------------------------------------------------
find_package(Sqlite)
if (NOT SQLITE_FOUND)
  message(FATAL_ERROR "Could not find SQLite")
endif (NOT SQLITE_FOUND)

# --------------------------------------------------------
# See this page for useful hints: https://cmake.org/Wiki/CMakeForFLTK

set (COMSEL_XLIBS  )

if(UNIX)
  find_package(X11)
  if (NOT X11_FOUND)
    message (FATAL_ERROR "Couldn't find X11: required for building csg")
  endif (NOT X11_FOUND)
  set (COMSEL_XLIBS
    #           Xaw Xmu Xpm Xt Xft X11 Xinerama dl fontconfig
    # Xext Xft fontconfig Xinerama dl m X11
    Xext Xft fontconfig dl m X11  Xfixes 
    # Xft dl m X11
    )
endif (UNIX)


include (${LOCAL_MODULE_DIR}/FLTKHelper.cmake)
find_package(FLTK REQUIRED)
if (NOT FLTK_FOUND)
  message (FATAL_ERROR "Could not find fltk: required for building csg")
endif (NOT FLTK_FOUND)

set (FLUID_DIR
  fluid)

# --------------------------------------------------------

if (ENABLE_EFENCE)
  find_package(efence)
  if (NOT EFENCE_FOUND)
    message(FATAL_ERROR "Could not find Electric Fence memory debugger")
  endif (NOT EFENCE_FOUND)
endif (ENABLE_EFENCE)

# ------------------------------------------------- 
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


# Ditto for "Findkgraph.cmake"
find_package(kgraph)
if(NOT KGRAPH_FOUND)
  message(FATAL_ERROR "Could not find kgraph")
endif(NOT KGRAPH_FOUND)


# Ditto for "Findsmp.cmake"
find_package(smp)
if(NOT SMP_FOUND)
  message(FATAL_ERROR "Could not find smp")
endif(NOT SMP_FOUND)
# -------------------------------------------------
# files to be built into a library go in libsrc/
# any application or testing code goes in src

include_directories (
  ${KUTILS_INCLUDE_DIR}
  ${KMODEL_INCLUDE_DIR}
  ${KGRAPH_INCLUDE_DIR}
  ${SMP_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/libsrc/
  ${PROJECT_SOURCE_DIR}/src/
  ${FLUID_DIR}
  ${FLTK_INCLUDE_DIR}
  ${SQLITE_INCLUDE_DIR}
  ${TINYXML2_INCLUDE_DIR}
  )


set(COMSELLIB_SRCS
  ${PROJECT_SOURCE_DIR}/libsrc/comsel.cpp
  )

add_library(comsel STATIC ${COMSELLIB_SRCS})

# -------------------------------------------------
# define a little library of comsel GUI stuff

set (COMSELGUI_FLS
  ${FLUID_DIR}/csmain.fl
  )

message (STATUS "Use these COMSELGUI_FLS: ${COMSELGUI_FLS}")

fltk_wrap_ui (comselGUI ${COMSELGUI_FLS})
add_library (comselGUI STATIC ${comselGUI_FLTK_UI_SRCS})

# --------------------------------------------------------

set (CSG_SRCS
  ${PROJECT_SOURCE_DIR}/src/democomsel.cpp
  )

set (CSG_LIBS
  ${FLTK_LIBRARIES}
  ${KGRAPH_LIBRARIES}
  comselGUI
  ${COMSEL_XLIBS}
  )

add_executable (csg 
  ${CSG_SRCS}
)
target_link_libraries(csg
  comsel
  ${CSG_LIBS}
  ${SMP_LIBRARIES}
  ${KMODEL_LIBRARIES}
  ${KUTILS_LIBRARIES}
  ${SQLITE_LIBRARIES}
  ${EFENCE_LIBRARIES}
  ${TINYXML2_LIBRARIES}
  )
  
# suppress the console window under Windows
if (WIN32)  
  if (MSVC)  
    set_target_properties(csg PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
    message (STATUS "Using /SUBSYSTEM:WINDOWS") 
  endif(MSVC) 
endif(WIN32)

# -------------------------------------------------
# show some useful status/debugging information
message (STATUS "Using PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR})
message (STATUS "SMP_INCLUDE_DIR: " ${SMP_INCLUDE_DIR})
message (STATUS "KMODEL_INCLUDE_DIR: " ${KMODEL_INCLUDE_DIR})
message (STATUS "KUTILS_INCLUDE_DIR: " ${KUTILS_INCLUDE_DIR})

#message (STATUS "SMP_LIBRARIES: " ${SMP_LIBRARIES})
#message (STATUS "KMODEL_LIBRARIES: " ${KMODEL_LIBRARIES})
#message (STATUS "KUTILS_LIBRARIES: " ${KUTILS_LIBRARIES})

message (STATUS "FLTK_INCLUDE_DIR: " ${FLTK_INCLUDE_DIR})

# -------------------------------------------------
# As of early 2014, C++11 is still not the default
# for g++, so I have to provide it here.
if (UNIX OR MINGW)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++11 ")
  if (ENABLE_EFFCPP)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weffc++ ")
  endif (ENABLE_EFFCPP)
endif(UNIX OR MINGW)

# ===========================================
# Copyright KAPSARC. Open Source MIT License 
# ===========================================
