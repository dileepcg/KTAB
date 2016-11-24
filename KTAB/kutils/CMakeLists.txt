# --------------------------------------------
# Copyright KAPSARC. Open source MIT License.
# --------------------------------------------
# The MIT License (MIT)
# 
# Copyright (c) 2015 King Abdullah Petroleum Studies and Research Center
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software
# and associated documentation files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom 
# the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
# BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# -------------------------------------------------

project(kutils)

cmake_minimum_required(VERSION 3.0)
cmake_policy(VERSION 3.0)
set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${PROJECT_SOURCE_DIR}/../cmakemodules
  )

set(LIBRARY_OUTPUT_PATH      ${PROJECT_SOURCE_DIR}/)
set(EXECUTABLE_OUTPUT_PATH   ${PROJECT_SOURCE_DIR}/)

# -------------------------------------------------
# basic check that this is a supported compiler/OS
# -------------------------------------------------
if (WIN32)
  message (STATUS "OS: WIN32 (generic Windows)")
  if (MSVC)
    message (STATUS "Compiler: MSVC")
  elseif(CMAKE_COMPILER_IS_GNUCXX)
    message (STATUS "Compiler: GCC")
  else(MSVC)
    message(SEND_ERROR "Unsupported Windows compiler")
  endif()
elseif(UNIX)
  message (STATUS "OS: UNIX (generic *nix)")
else()
  message(SEND_ERROR "Unsupported OS")
endif(WIN32)
# -------------------------------------------------

if (UNIX)
  set (ENABLE_EFFCPP false CACHE  BOOL "Check Effective C++ Guidelines")
  set (ENABLE_EFENCE false CACHE  BOOL "Use Electric Fence memory debugger")
endif(UNIX)
# -------------------------------------------------
# find libraries on which this project depends


if (ENABLE_EFENCE)
  find_package(efence)
  if (NOT EFENCE_FOUND)
    message(FATAL_ERROR "Could not find Electric Fence memory debugger")
  endif (NOT EFENCE_FOUND)
endif (ENABLE_EFENCE) 

# -------------------------------------------------
# files to be built into a library go in libsrc/
# any application or testing code goes in src

include_directories(
  ${PROJECT_SOURCE_DIR}/libsrc/
  ${EFENCE_INCLUDE_DIR} 
)

set(KTABBASIC_SRCS
  libsrc/kutils.cpp
  libsrc/prng.cpp
  libsrc/gaopt.cpp
  libsrc/kmatrix.cpp
  libsrc/hcsearch.cpp
  libsrc/vimcp.cpp
)

add_library(kutils STATIC ${KTABBASIC_SRCS})
 
# -------------------------------------------------
# Under Linux, default start of DESTINATION path is /usr/local/ 
# so "ktab/lib" denotes destination /usr/local/ktab/lib
# Under Windows, default start of DESTINATION path is "C:\Program Files\kutils\"
# so "ktab/lib" denotes destination C:\Program Files\kutils\ktab\lib

set (KTAB_INSTALL_DIR)
if (WIN32)
  set (KTAB_INSTALL_DIR
  c:/local/ktab
  )
endif(WIN32)
if (UNIX)
  set (KTAB_INSTALL_DIR
  /usr/local/ktab
  )
endif(UNIX)

install(
  TARGETS
    kutils
  DESTINATION 
    ${KTAB_INSTALL_DIR}/lib)  

install(
  FILES
    libsrc/kutils.h  
    libsrc/gaopt.h  
    libsrc/hcsearch.h  
    libsrc/kmatrix.h  
    libsrc/prng.h  
    libsrc/vimcp.h
  DESTINATION
    ${KTAB_INSTALL_DIR}/include)

# -------------------------------------------------
# demo program

include_directories(
  ${PROJECT_SOURCE_DIR}/libsrc/
  ${PROJECT_SOURCE_DIR}/src/ 
)

set(UTILDEMO_SRCS
 src/demo.cpp
 )
 

add_executable(demoutils
  ${UTILDEMO_SRCS}
  )

target_link_libraries(demoutils
  kutils 
  ${EFENCE_LIBRARIES}
  )
# -------------------------------------------------
# show some useful status/debugging information

message(STATUS "CMAKE_SYSTEM_NAME: " ${CMAKE_SYSTEM_NAME} )
message(STATUS "Using PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR})
message(STATUS "KTAB_INSTALL_DIR: " ${KTAB_INSTALL_DIR})
# -------------------------------------------------
# As of early 2014, C++11 is still not the default
# for g++, so I have to provide it here.
if (UNIX OR MINGW)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++11 ")
  if (ENABLE_EFFCPP)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weffc++ ")
  endif (ENABLE_EFFCPP)
endif(UNIX OR MINGW)

# --------------------------------------------
# Copyright KAPSARC. Open source MIT License.
# --------------------------------------------

