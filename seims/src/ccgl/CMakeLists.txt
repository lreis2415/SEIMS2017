# ----------------------------------------------------------------------------
#  Root CMake file for CCGL.
#     A simple Common Cross-platform Geographic Library.
#        + Basic operations on array, file system, string, and time, etc.
#        + Raster data class with masking and decomposition features with support of GDAL
#        + Data management with support of MongoDB
#
#     Please build from the off-tree build directory, e.g.:
#       $ mkdir -p build
#       $ cd build
#       $ cmake .. -DUNITTEST=1
#       $ make
#       $ ctest --rerun-failed --output-on-failure
#     Optional cmake options:
#       -DUNITTEST=1  means run unittest based gtest/gmock
#       -DCODE_COVERAGE=1 means run code coverage based GCC (gcov and lcov) and Clang(llvm-cov and llvm-profdata)
#       -DLLVM_ROOT_DIR Specific the root directory of brew installed LLVM, e.g., /opt/homebrew/opt/llvm
#       -DBUILD_DOC=1 means build CCGL documentation based on doxygen
#
#       Sanitizers related flags (Experimental):
#
#       -DSANITIZE_ADDRESS=On for MSVC and Clang
#       -DSANITIZE_MEMORY=On for MSVC, GCC, and Clang
#       -DSANITIZE_THREAD=On for MSVC, GCC, and Clang
#       -DSANITIZE_UNDEFINED=On for MSVC, GCC, and Clang
#
#  Routine testing platforms and compilers include:
#     1. Windows 10 with Visual Studio 2010/2015/2019, mongo-c-driver-1.16.2, GDAL-1.11.4/2.4.4/3.3.3
#     2. CentOS 6.2 (cluster) with GCC-4.8.4, mongo-c-driver-1.5.5, GDAL-1.9.0
#     3. Ubuntu 16.04 (VMware) with GCC-5.4.0, mongo-c-driver-1.6.3, GDAL-2.4.4
#     4. macOS 13.x with latest Clang (Xcode built-in and brew installed) and latest GCC (brew installed),
#              mongo-c-driver-1.16.2, GDAL-3.3.3 (brew installed and framework by kyngchaos.com)
#
#  Created and maintained by Liangjun Zhu, zlj(a)lreis.ac.cn
#  Latest updated: Aug 10, 2023
#  Copyright (C) 2017-2023 Lreis, IGSNRR, CAS
# ----------------------------------------------------------------------------
### Disable in-source builds to prevent source tree corruption.
IF (" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
    MESSAGE(FATAL_ERROR "
FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.")
ENDIF ()
CMAKE_MINIMUM_REQUIRED(VERSION 3.10 FATAL_ERROR)

cmake_policy(SET CMP0015 NEW)
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0074 NEW)
cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0079 NEW)
SET(CMAKE_SH "CMAKE_SH-NOTFOUND")

### Set coding language
PROJECT(CCGL CXX C)

### Project name and output library name
PROJECT(CCGL)
SET(CCGLNAME ccgl)

### In case of Makefiles if the user does not setup CMAKE_BUILD_TYPE, assume it's Release.
IF(CMAKE_GENERATOR MATCHES "Makefiles|Ninja" AND "${CMAKE_BUILD_TYPE}" STREQUAL "")
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
SET(DOC_INSTALL_DIR "${INSTALL_DIR}/doc" CACHE PATH "Set path cache to the documentation")

### Allow project folders in MSVC.
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
    add_definitions(-DMACOSX)
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
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
ENDIF()

### Break in case of popular CMake configuration mistakes.
IF(NOT CMAKE_SIZEOF_VOID_P GREATER 0)
    MESSAGE(FATAL_ERROR "CMake fails to determine the bitness of the target platform.
  Please check your CMake and compiler installation.")
ENDIF()

# Global flag to cause add_library to create shared libraries if on, otherwise static library.
option(BUILD_SHARED_LIBS "Build shared libraries (DLLs)." OFF)
# libraries built with hunter link against the dynamic CRT on windows
option(BUILD_WITH_STATIC_CRT "Build against dynamic CRT on windows." ON)
# Build documentation by doxygen
option(BUILD_DOC "Build CCGL documentation." OFF)

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

### Find GDAL which is optional.
FIND_PACKAGE(GDAL)

### Find mongo-c-driver which is optional
IF(BSON_ROOT)
  SET(ENV{BSON_ROOT} ${BSON_ROOT})
  SET(ENV{BSON_DIR} ${BSON_ROOT})
  SET(ENV{BSON_ROOT_DIR} ${BSON_ROOT})
ENDIF()
IF(MONGOC_ROOT)
  SET(ENV{MONGOC_ROOT} ${MONGOC_ROOT})
  SET(ENV{MONGOC_DIR} ${MONGOC_ROOT})
  SET(ENV{MONGOC_ROOT_DIR} ${MONGOC_ROOT})
ENDIF()
INCLUDE(FindMongoC)
INCLUDE(FindBson)

### CCGL library
SET(CCGL_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
SET(CCGL_DIR ${CCGL_BASE_DIR}/src)
SET(APPS_DIR ${CCGL_BASE_DIR}/apps)
SET(TEST_DIR ${CCGL_BASE_DIR}/test)

FILE(GLOB CCGL_SRCS ${CCGL_DIR}/*.cpp ${CCGL_DIR}/*.hpp ${CCGL_DIR}/*.h)
FILE(GLOB INC_FILES ${CCGL_DIR}/*.hpp ${CCGL_DIR}/*.h)

ADD_LIBRARY(${CCGLNAME} ${CCGL_SRCS})
SET_TARGET_PROPERTIES(${CCGLNAME} PROPERTIES PUBLIC_HEADER "${INC_FILES}")
GET_DIRECTORY_PROPERTY(hasParent PARENT_DIRECTORY)
IF (hasParent)
    SET(CCGLNAME "${CCGLNAME}" PARENT_SCOPE)
ENDIF ()

set(target_code_coverage_PUBLIC 1)
set(TARGET_VISIBILITY PUBLIC)

IF(OPENMP_FOUND)
  SET(WITH_OPENMP 1)
  ADD_DEFINITIONS(-DSUPPORT_OMP)
  INCLUDE(SetFlagOpenMP)
  ### For Xcode generator, specific LLVM-Clang installed by brew, and add link library of OpenMP explicitly.
  IF(CV_CLANG AND LLVM_VERSION_MAJOR)
      TARGET_LINK_LIBRARIES(${CCGLNAME} ${TARGET_VISIBILITY} ${OpenMP_LIBRARY})
  ENDIF()
  MESSAGE(STATUS "Compiling with OpenMP...")
ENDIF()

IF (GDAL_FOUND)
  SET(WITH_GDAL 1)
  ADD_DEFINITIONS(-DUSE_GDAL)
  INCLUDE(SetFlagGDAL)
  geo_include_directories(${GDAL_INCLUDE_DIR})
  TARGET_LINK_LIBRARIES(${CCGLNAME} ${TARGET_VISIBILITY} ${GDAL_LIBRARIES})
  SET_TARGET_PROPERTIES(${CCGLNAME} PROPERTIES LINKER_LANGUAGE CXX)
  MESSAGE(STATUS "Compiling with GDAL...")
ENDIF ()

IF (BSON_FOUND AND MONGOC_FOUND)
  SET(WITH_MONGOC 1)
  ADD_DEFINITIONS(-DUSE_MONGODB)
  geo_include_directories(${BSON_INCLUDE_DIR} ${MONGOC_INCLUDE_DIR})
  TARGET_LINK_LIBRARIES(${CCGLNAME} ${TARGET_VISIBILITY} ${BSON_LIBRARIES} ${MONGOC_LIBRARIES})
  MESSAGE(STATUS "Compiling with mongo-c-driver...")
ENDIF ()

geo_include_directories(${CCGL_DIR})

IF(UNIX)
    IF(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_ICC)
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
        geo_list_unique(CMAKE_C_FLAGS)
        geo_list_unique(CMAKE_CXX_FLAGS)
    ENDIF()
ENDIF()

### Code coverage requires UNITTEST.
IF ((CV_GCC OR CV_CLANG) AND CODE_COVERAGE)
    SET(UNITTEST 1)
ENDIF ()

### Check if use Sanitizers for GCC and Clang
SET(USE_SAN 0)
IF (SANITIZE_ADDRESS OR SANITIZE_MEMORY OR SANITIZE_THREAD OR SANITIZE_UNDEFINED)
  SET(USE_SAN 1)
  find_package(Sanitizers)
ENDIF ()


### Build Googletest if UNITTEST is set as True or 1
IF (UNITTEST STREQUAL 1)
    ADD_DEFINITIONS(-DUNITTEST)
    ### unittest based on gtest/gmock, which is optional.
    SET(GTestTAG release-1.8.1)
    IF(MSVC_VERSION LESS 1900)
        SET(GTestTAG release-1.8.0)
    ENDIF()
    MESSAGE(STATUS "GoogleTest Version: ${GTestTAG}")
    geo_include_directories(${CCGL_BASE_DIR})
    ### Download and compile googletest using FetchContent supported officially by CMake 3.11+
    # https://google.github.io/googletest/quickstart-cmake.html
    # Because download from github often failed, I decided to include googletest tags in src directory.
    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL "file://${CCGL_BASE_DIR}/src/googletest_tags_zip/${GTestTAG}.zip"
      #URL https://github.com/google/googletest/archive/refs/tags/${GTestTAG}.zip
      )
    IF (WIN32)
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    ENDIF ()
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(BUILD_GTEST ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
ENDIF ()

IF ((CV_GCC OR CV_CLANG) AND CODE_COVERAGE)
    include(code-coverage)
    #add_code_coverage() # Adds instrumentation to all targets
    # Instead, add coverage via target commands, in test/CMakeLists.txt
ENDIF ()

### Copy test data to install directory!
FILE(MAKE_DIRECTORY ${INSTALL_DIR})
FILE(COPY ${CCGL_BASE_DIR}/data/ DESTINATION ${INSTALL_DIR}/share/ccgl)

IF (UNITTEST STREQUAL 1)
    # Enable testing for the project
    enable_testing()
    ADD_SUBDIRECTORY(${TEST_DIR})
    # For the convenient of debugging in MSVC and CLion
    IF (CMAKE_GENERATOR MATCHES "Visual Studio" OR CMAKE_GENERATOR MATCHES "Xcode")
        FILE(COPY ${CCGL_BASE_DIR}/data DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test/Debug)
        FILE(COPY ${CCGL_BASE_DIR}/data DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test/Release)
    ELSE ()
        FILE(COPY ${CCGL_BASE_DIR}/data DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test)
    ENDIF ()
ENDIF ()

### CCGL based applications
ADD_SUBDIRECTORY(${APPS_DIR})

# Install CCGL, including headers, libraries, executable, stc.
INSTALL(TARGETS ${CCGLNAME}
        RUNTIME DESTINATION "${INSTALL_DIR}/bin"
        PUBLIC_HEADER DESTINATION "${INSTALL_DIR}/include/ccgl"
        ARCHIVE DESTINATION "${INSTALL_DIR}/lib"
        LIBRARY DESTINATION "${INSTALL_DIR}/lib")

### Build doc based on doxygen
IF(BUILD_DOC)
  ADD_SUBDIRECTORY(./doc)
ENDIF()

### Build platform.
STATUS("")
STATUS("  Platform:")
IF(NOT CMAKE_VERSION VERSION_LESS 2.8.11 AND NOT BUILD_INFO_SKIP_TIMESTAMP)
    STRING(TIMESTAMP TIMESTAMP "" UTC)
    IF(TIMESTAMP)
        STATUS("    Timestamp:"  ${TIMESTAMP})
    ENDIF()
ENDIF()
STATUS("    Host:"               ${CMAKE_HOST_SYSTEM_NAME} ${CMAKE_HOST_SYSTEM_VERSION} ${CMAKE_HOST_SYSTEM_PROCESSOR})
IF(CMAKE_CROSSCOMPILING)
    STATUS("    Target:"         ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION} ${CMAKE_SYSTEM_PROCESSOR})
ENDIF()
STATUS("    CMake:"              ${CMAKE_VERSION})
STATUS("    CMake generator:"    ${CMAKE_GENERATOR})
STATUS("    CMake build tool:"   ${CMAKE_BUILD_TOOL})
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
STATUS("    Use OpenMP:"  OPENMP_FOUND   THEN "YES" ELSE "NO")
STATUS("    Use GDAL:"    GDAL_FOUND     THEN "YES (LIB: ${GDAL_LIBRARIES}, INC: ${GDAL_INCLUDE_DIR})" ELSE "NO")
STATUS("    Use BSON:"    BSON_FOUND     THEN "YES (LIB: ${BSON_LIBRARIES}, INC: ${BSON_INCLUDE_DIR})" ELSE "NO")
STATUS("    Use MongoC:"  MONGOC_FOUND   THEN "YES (LIB: ${MONGOC_LIBRARIES}, INC: ${MONGOC_INCLUDE_DIR})" ELSE "NO")

STATUS("    Run Doxygen:"      BUILD_DOC THEN "YES" ELSE "NO")
STATUS("    Run CodeCoverage:" CODE_COVERAGE THEN "YES (GCOV: ${GCOV_PATH}, LCOV: ${LCOV_PATH}, LLVM_COV: ${LLVM_COV_PATH})" ELSE "NO")

### Auxiliary.
STATUS("")
STATUS("  Install path:" ${INSTALL_DIR})
STATUS("")
