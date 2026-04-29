cmake_minimum_required(VERSION 3.20)

add_executable(FileTest
    ${CMAKE_SOURCE_DIR}/AceCondition.cpp
    ${CMAKE_SOURCE_DIR}/AceResource.cpp
    ${CMAKE_SOURCE_DIR}/DateTime.cpp
    ${CMAKE_SOURCE_DIR}/DlgCopyFile.cpp
    ${CMAKE_SOURCE_DIR}/DlgDataEditor.cpp
    ${CMAKE_SOURCE_DIR}/DlgEaEditor.cpp
    ${CMAKE_SOURCE_DIR}/DlgEasEditor.cpp
    ${CMAKE_SOURCE_DIR}/DlgFileTest.cpp
    ${CMAKE_SOURCE_DIR}/DlgFlags.cpp
    ${CMAKE_SOURCE_DIR}/DlgPrivileges.cpp
    ${CMAKE_SOURCE_DIR}/DlgSimple.cpp
    ${CMAKE_SOURCE_DIR}/NtStatus2Text.cpp
    ${CMAKE_SOURCE_DIR}/Page00Transaction.cpp
    ${CMAKE_SOURCE_DIR}/Page01Create.cpp
    ${CMAKE_SOURCE_DIR}/Page02NtCreate.cpp
    ${CMAKE_SOURCE_DIR}/Page03ReadWrite.cpp
    ${CMAKE_SOURCE_DIR}/Page04Mapping.cpp
    ${CMAKE_SOURCE_DIR}/Page05FileOps.cpp
    ${CMAKE_SOURCE_DIR}/Page06NtFileInfo.cpp
    ${CMAKE_SOURCE_DIR}/Page07NtFsInfo.cpp
    ${CMAKE_SOURCE_DIR}/Page08Ea.cpp
    ${CMAKE_SOURCE_DIR}/Page09Security.cpp
    ${CMAKE_SOURCE_DIR}/Page10Links.cpp
    ${CMAKE_SOURCE_DIR}/Page11Streams.cpp
    ${CMAKE_SOURCE_DIR}/Page12Ioctl.cpp
    ${CMAKE_SOURCE_DIR}/ReparseDataHsm.cpp
    ${CMAKE_SOURCE_DIR}/TestCode.cpp
    ${CMAKE_SOURCE_DIR}/TAceHelper.cpp
    ${CMAKE_SOURCE_DIR}/TDataEditor.cpp
    ${CMAKE_SOURCE_DIR}/TToolTip.cpp
    ${CMAKE_SOURCE_DIR}/Utils.cpp
    ${CMAKE_SOURCE_DIR}/WinMain.cpp
    ${CMAKE_SOURCE_DIR}/FileTest.rc
)

add_definitions(-D_AFXDLL)
set(CMAKE_MFC_FLAG 2)

# Library paths
# Try lib/Aaa first (as per README), fallback to ../aaa if not found
if(EXISTS "${CMAKE_SOURCE_DIR}/lib/Aaa")
    get_filename_component(LIB_AAA_ROOT "${CMAKE_SOURCE_DIR}/lib/Aaa" ABSOLUTE)
elseif(EXISTS "${CMAKE_SOURCE_DIR}/../aaa")
    get_filename_component(LIB_AAA_ROOT "${CMAKE_SOURCE_DIR}/../aaa" ABSOLUTE)
else()
    # Default to lib/Aaa structure
    get_filename_component(LIB_AAA_ROOT "${CMAKE_SOURCE_DIR}/lib/Aaa" ABSOLUTE)
endif()


target_link_options(FileTest PRIVATE "/SUBSYSTEM:WINDOWS")

target_include_directories(FileTest PRIVATE "${LIB_AAA_ROOT}/inc")

# Make sure that CRT library is statically linked
if(MSVC)
    target_compile_options(FileTest PUBLIC /MT)
endif()

# Suppress -Wwrite-strings warning for MINGW/GCC builds
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(FileTest PRIVATE -Wwrite-strings)
endif()

message(CMAKE_GENERATOR_PLATFORM="${CMAKE_GENERATOR_PLATFORM}")

if(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
    set(LIB_AAA_VER "lib32")
else()
    set(LIB_AAA_VER "lib64")
endif()

target_link_libraries(FileTest PRIVATE
    kernel32.lib
    shell32.lib
    Comctl32.lib
    Comdlg32.lib
    Version.lib
    ${LIB_AAA_ROOT}/${LIB_AAA_VER}/ntdll.lib
    ${LIB_AAA_ROOT}/${LIB_AAA_VER}/UtilsRUS.lib
    )

project(FileTest)
