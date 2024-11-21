# ----------------------------------------------------------------------------
#  Root CMake file for extended TauDEM.
#
#     Please from the off-tree build directory, invoke:
#       $ cmake <path/to/TauDEM_ext>
#
#       Optional cmake options:
#         -DINSTALL_PREFIX=<path/to/install>
#         -DLLVM_ROOT_DIR Specific the root directory of brew installed LLVM, e.g., /usr/local/opt/llvm
#
#  Routine testing platforms and compilers include:
#     1. Windows 10 with Visual Studio 2010/2015/2019, MSMPI-v8.1, GDAL-1.11.4/2.4.4/3.3.3
#     2. CentOS 6.2 (cluster) with GCC-4.8.4, MPICH-3.1.4, GDAL-1.9.0
#     3. Ubuntu 16.04 (VMware) with GCC-5.4.0, GDAL-2.4.4, OpenMPI-3.x
#     4. macOS 12.1 with AppleClang-13.0 (Xcode built-in and brew installed) and GCC-11.2 (brew installed),
#              GDAL-3.4.0 (brew installed), OpenMPI-4.1.2
#
#  Created and maintained by Liang-Jun Zhu (zlj@lreis.ac.cn)
#  Latest updated: Aug. 15, 2023
#  Copyright (C) 2017-2023 Lreis, IGSNRR, CAS
# ----------------------------------------------------------------------------
### Disable in-source builds to prevent source tree corruption.
IF(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  MESSAGE(FATAL_ERROR "
FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.")
ENDIF()
CMAKE_MINIMUM_REQUIRED(VERSION 3.10 FATAL_ERROR)

cmake_policy(SET CMP0015 NEW)
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0074 NEW)
cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0079 NEW)
SET(CMAKE_SH "CMAKE_SH-NOTFOUND")

# Set initial project name with coding languages
PROJECT(TauDEM_ext CXX C)
### TauDEM_ext compilation version.
SET(PRJNAME "TauDEM_ext")

### In case of Makefiles if the user does not setup CMAKE_BUILD_TYPE, assume it's Release.
IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
  SET(CMAKE_BUILD_TYPE Release)
ENDIF()

### Build Postfix
SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "Default Debug postfix")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "Default Release postfix")
SET(CMAKE_RELWITHDEBINFO_POSTFIX "rd" CACHE STRING "Default RelWithDebInfo postfix")
SET(CMAKE_MINSIZEREL_POSTFIX "s" CACHE STRING "Default MinSizeRel postfix")
IF (CMAKE_BUILD_TYPE MATCHES "Release")
  SET(CMAKE_BUILD_POSTFIX "${CMAKE_RELEASE_POSTFIX}")
ELSEIF (CMAKE_BUILD_TYPE MATCHES "MinSizeRel")
  SET(CMAKE_BUILD_POSTFIX "${CMAKE_MINSIZEREL_POSTFIX}")
ELSEIF(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
  SET(CMAKE_BUILD_POSTFIX "${CMAKE_RELWITHDEBINFO_POSTFIX}")
ELSEIF(CMAKE_BUILD_TYPE MATCHES "Debug")
  SET(CMAKE_BUILD_POSTFIX "${CMAKE_DEBUG_POSTFIX}")
ELSE()
  SET(CMAKE_BUILD_POSTFIX "")
ENDIF()

### Set cmake find path
set(CMAKE_MODULE_PATH
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake
        ${CMAKE_MODULE_PATH}
   )

### Set default or specified installation directory.
IF (INSTALL_PREFIX)
    SET(INSTALL_DIR ${INSTALL_PREFIX} CACHE PATH "Set specified install path" FORCE)
ELSE()
    IF (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        SET(INSTALL_DIR ${PROJECT_BINARY_DIR} CACHE PATH "Set specified install path" FORCE)
    ELSE ()
        SET(INSTALL_DIR ${CMAKE_INSTALL_PREFIX} CACHE PATH "Set default install path" FORCE)
    ENDIF ()
ENDIF ()
SET(INSTALL_PREFIX ${INSTALL_DIR} CACHE PATH "Set INSTALL_PREFIX cache" FORCE)
SET(CMAKE_INSTALL_PREFIX ${INSTALL_DIR} CACHE PATH "Set CMAKE_INSTALL_PREFIX cache" FORCE)

### Allow project folders in MSVC and Xcode.
IF(MSVC OR XCODE)
  SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
ENDIF()

### OS platform.
if (WIN32)
    add_definitions(-DWINDOWS)
    if (MSVC)
        add_definitions(-DMSVC)
    endif ()
elseif (APPLE)
    add_definitions(-DMACOS)
    if (XCODE)
        add_definitions(-DXCODE)
    endif ()
else ()
    add_definitions(-DLINUX)
endif ()

### Assume built-in pthreads on macOS X
IF(APPLE)
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WI N32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
ENDIF()

### Break in case of popular CMake configuration mistakes.
IF(NOT CMAKE_SIZEOF_VOID_P GREATER 0)
  MESSAGE(FATAL_ERROR "CMake fails to determine the bitness of the target platform.
  Please check your CMake and compiler installation."
)
ENDIF()

### Detect compiler and target platform architecture.
INCLUDE(Utils)
SET(ENABLE_CXX11 1)
INCLUDE(DetectCXXCompiler)
### Use statically or dynamically linked CRT? Default: dynamic
IF(MSVC)
    INCLUDE(CRTLinkage)
ENDIF(MSVC)

### Add standard paths or specified paths for Find libraries and headers.
INCLUDE(AddFindPaths)

### Use GNU standard installation directories
INCLUDE(GNUInstallDirs)

set(TARGET_VISIBILITY )

### Find packages.

### Find GDAL which is required.
FIND_PACKAGE(GDAL REQUIRED)
IF (GDAL_FOUND)
  SET(WITH_GDAL 1)
  INCLUDE(SetFlagGDAL)
ELSE ()
  MESSAGE(FATAL_ERROR "FATAL: Could not find GDAL!")
ENDIF ()

### Find MPI which is required
FIND_PACKAGE(MPI REQUIRED)
IF(MPI_FOUND)
  SET(WITH_MPI 1)
  INCLUDE(SetFlagMPI)
  SET(PRJNAME ${PRJNAME}_MPI)
  MESSAGE(STATUS "Compiling with MPI...")
ELSE()
  MESSAGE(FATAL_ERROR "FATAL: Could not find MPI, which is required for TauDEM_ext!")
ENDIF()

### Find OMP which is optional
# Refers to https://mac.r-project.org/openmp/ for further method to handle openmp in AppleClang
IF(NOT CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
  IF(CV_CLANG)
    message(STATUS "Find OpenMP from LLVM")
    # To use OpenMP in Clang built by CMake, the recommended way is to
    #   install llvm alone by 'brew install llvm', which has builtin 'libomp'
    IF (NOT LLVM_ROOT_DIR)
        SET(LLVM_ROOT_DIR "/opt/homebrew/opt/llvm")
    ENDIF ()
    FIND_PACKAGE(LLVM)
    IF(LLVM_VERSION_MAJOR)
        geo_include_directories(${LLVM_LIBRARY_DIRS}/clang/${LLVM_VERSION_BASE_STRING}/include)
        SET(OpenMP_CXX_FLAGS "-fopenmp")
        SET(OpenMP_C_FLAGS "-fopenmp")
        FIND_LIBRARY(OpenMP_LIBRARY
            NAMES "omp"
            HINTS ${LLVM_LIBRARY_DIRS})
        SET(OPENMP_FOUND 1)
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LLVM_LDFLAGS}")
    ENDIF ()
  ELSE ()
    FIND_PACKAGE(OpenMP)
  ENDIF ()
ENDIF ()

IF(OPENMP_FOUND)
  SET(WITH_OPENMP 1)
  SET(PRJNAME ${PRJNAME}_OMP)
  ADD_DEFINITIONS(-DSUPPORT_OMP)
  INCLUDE(SetFlagOpenMP)
  MESSAGE(STATUS "Compiling with OpenMP...")
ENDIF()

PROJECT(${PRJNAME})

SET(TAUDEM_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)
SET(TAUDEM_EXT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src_ext)

geo_include_directories(${GDAL_INCLUDE_DIR} ${MPI_INCLUDE_PATH})
ADD_SUBDIRECTORY(${TAUDEM_SRC})
ADD_SUBDIRECTORY(${TAUDEM_EXT_SRC})

### Build platform.
STATUS("")
STATUS("  Platform:")
IF(NOT CMAKE_VERSION VERSION_LESS 2.8.11 AND NOT BUILD_INFO_SKIP_TIMESTAMP)
  STRING(TIMESTAMP TIMESTAMP "" UTC)
  IF(TIMESTAMP)
    STATUS("    Timestamp:"    ${TIMESTAMP})
  ENDIF()
ENDIF()
STATUS("    Host:"             ${CMAKE_HOST_SYSTEM_NAME} ${CMAKE_HOST_SYSTEM_VERSION} ${CMAKE_HOST_SYSTEM_PROCESSOR})
IF(CMAKE_CROSSCOMPILING)
  STATUS("    Target:"         ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION} ${CMAKE_SYSTEM_PROCESSOR})
ENDIF()
STATUS("    CMake:"            ${CMAKE_VERSION})
STATUS("    CMake generator:"  ${CMAKE_GENERATOR})
STATUS("    CMake build tool:" ${CMAKE_BUILD_TOOL})
IF(MSVC)
  STATUS("    MSVC:"           ${MSVC_VERSION})
ENDIF()
IF(CMAKE_GENERATOR MATCHES Xcode)
  STATUS("    Xcode:"          ${XCODE_VERSION})
ENDIF()
IF(NOT CMAKE_GENERATOR MATCHES "Xcode|Visual Studio")
  STATUS("    Configuration:"  ${CMAKE_BUILD_TYPE})
ENDIF()

### C/C++ options.
IF(CMAKE_CXX_COMPILER_VERSION)
  SET(GEO_COMPILER_STR "${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1} (ver ${CMAKE_CXX_COMPILER_VERSION})")
ELSE()
  SET(GEO_COMPILER_STR "${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1}")
ENDIF()
STRING(STRIP "${GEO_COMPILER_STR}" GEO_COMPILER_STR)

STATUS("")
STATUS("  C/C++:")
IF(ENABLE_CXX11 OR HAVE_CXX11)
STATUS("    C++11:" HAVE_CXX11 THEN YES ELSE NO)
ENDIF()
STATUS("    C++ Compiler:"           ${GEO_COMPILER_STR})
STATUS("    C++ flags (Release):"    ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE})
STATUS("    C++ flags (Debug):"      ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG})
STATUS("    C Compiler:"             ${CMAKE_C_COMPILER} ${CMAKE_C_COMPILER_ARG1})
STATUS("    C flags (Release):"      ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE})
STATUS("    C flags (Debug):"        ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG})
IF(WIN32)
  STATUS("    Linker flags (Release):" ${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_RELEASE})
  STATUS("    Linker flags (Debug):"   ${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_DEBUG})
ELSE()
  STATUS("    Linker flags (Release):" ${CMAKE_SHARED_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS_RELEASE})
  STATUS("    Linker flags (Debug):"   ${CMAKE_SHARED_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS_DEBUG})
ENDIF()

### Dependencies.
STATUS("")
STATUS("    Use MPI:"     MPI_FOUND      THEN "YES (LIB: ${MPI_LIBRARIES}, INC: ${MPI_INCLUDE_PATH})" ELSE "NO")
STATUS("    Use OpenMP:"  OPENMP_FOUND   THEN "YES" ELSE "NO")
STATUS("    Use GDAL:"    GDAL_FOUND     THEN "YES (LIB: ${GDAL_LIBRARIES}, INC: ${GDAL_INCLUDE_DIR})" ELSE "NO")

### Auxiliary.
STATUS("")
STATUS("  Install path:" ${INSTALL_DIR})
STATUS("")
