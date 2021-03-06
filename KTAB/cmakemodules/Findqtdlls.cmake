# --------------------------------------------
# Copyright KAPSARC. Open source MIT License.
# --------------------------------------------

# Finding Qt Compiled Libraries Root Directory - for CMAKE_PREFIX_PATH

#if(WIN32)
#    find_path(PREFIX_PATH lib/cmake PATHS $ENV{PATH}\\..)
#    set(PREFIX_PATH ${PREFIX_PATH}\\lib\\cmake)
#endif(WIN32)

#------------------------------------------------------------------
set(CMAKE_FIND_LIBRARY_SUFFIXES "58.dll" "57.dll" "56.dll" "55.dll" "54.dll" "53.dll" "52.dll" "51.dll")

find_library(ICUIN_LIB  NAMES "icuin"  PATHS  ${PREFIX_PATH}/../bin/)
find_library(ICUUC_LIB  NAMES "icuuc"  PATHS  ${PREFIX_PATH}/../bin/)
find_library(ICUDT_LIB  NAMES "icudt"  PATHS  ${PREFIX_PATH}/../bin/)

#------------------------------------------------------------------
# Copying Qt Dlls to smp\smpq_bin directory

if(WIN32)

  ADD_CUSTOM_COMMAND (TARGET smpq POST_BUILD
    # DEBUG
    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../bin/Qt5Cored.dll          ${EXECUTABLE_OUTPUT_PATH}smpq_bin
    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../bin/Qt5Guid.dll           ${EXECUTABLE_OUTPUT_PATH}smpq_bin
    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../bin/Qt5Widgetsd.dll       ${EXECUTABLE_OUTPUT_PATH}smpq_bin
    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../bin/Qt5Sqld.dll           ${EXECUTABLE_OUTPUT_PATH}smpq_bin
    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../bin/Qt5PrintSupportd.dll  ${EXECUTABLE_OUTPUT_PATH}smpq_bin

    #plugins- windows
    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../plugins/platforms/qwindowsd.dll
    ${EXECUTABLE_OUTPUT_PATH}smpq_bin/plugins/platforms

    #plugins - sqlite
    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../plugins/sqldrivers/qsqlited.dll
    ${EXECUTABLE_OUTPUT_PATH}smpq_bin/plugins/sqldrivers

    #OTHER LIBS
    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../bin/libEGLd.dll           ${EXECUTABLE_OUTPUT_PATH}smpq_bin
    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../bin/libGLESv2d.dll        ${EXECUTABLE_OUTPUT_PATH}smpq_bin

    # RELEASE
    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../bin/Qt5Core.dll           ${EXECUTABLE_OUTPUT_PATH}smpq_bin
    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../bin/Qt5Gui.dll            ${EXECUTABLE_OUTPUT_PATH}smpq_bin
    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../bin/Qt5Widgets.dll        ${EXECUTABLE_OUTPUT_PATH}smpq_bin
    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../bin/Qt5Sql.dll            ${EXECUTABLE_OUTPUT_PATH}smpq_bin
    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../bin/Qt5PrintSupport.dll   ${EXECUTABLE_OUTPUT_PATH}smpq_bin

    #plugins - windows
    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../plugins/platforms/qwindows.dll
    ${EXECUTABLE_OUTPUT_PATH}smpq_bin/plugins/platforms

    #plugins - sqlite
    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../plugins/sqldrivers/qsqlite.dll
    ${EXECUTABLE_OUTPUT_PATH}smpq_bin/plugins/sqldrivers

    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../bin/libEGL.dll            ${EXECUTABLE_OUTPUT_PATH}smpq_bin
    COMMAND ${CMAKE_COMMAND} -E copy ${PREFIX_PATH}/../../bin/libGLESv2.dll         ${EXECUTABLE_OUTPUT_PATH}smpq_bin

    #icu Libs
    COMMAND ${CMAKE_COMMAND} -E copy ${ICUIN_LIB}            ${EXECUTABLE_OUTPUT_PATH}smpq_bin
    COMMAND ${CMAKE_COMMAND} -E copy ${ICUUC_LIB}            ${EXECUTABLE_OUTPUT_PATH}smpq_bin
    COMMAND ${CMAKE_COMMAND} -E copy ${ICUDT_LIB}            ${EXECUTABLE_OUTPUT_PATH}smpq_bin

    # Output Message
    COMMENT "Copying Qt binaries" VERBATIM)

endif(WIN32)

# --------------------------------------------
# Copyright KAPSARC. Open source MIT License.
# --------------------------------------------
