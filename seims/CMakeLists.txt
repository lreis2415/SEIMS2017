### Use MongoDB as database if available
IF (BSON_FOUND AND MONGOC_FOUND)
  ADD_DEFINITIONS(-DMONGO_HAVE_STDINT -DUSE_MONGODB)
ENDIF ()

##############  Set CCGL               ###############
SET(CCGL_LIB ${CMAKE_CURRENT_SOURCE_DIR}/src/ccgl)
SET(CCGL_INC ${CCGL_LIB}/src)
FILE(GLOB CCGL_SRC ${CCGL_INC} *.h *.cpp)
##############  Stand-alone programs   ###############
SET(FIELD_PARTITION ${CMAKE_CURRENT_SOURCE_DIR}/src/field_partition)
SET(GRID_LAYERING ${CMAKE_CURRENT_SOURCE_DIR}/src/grid_layering)
SET(IUH ${CMAKE_CURRENT_SOURCE_DIR}/src/iuh)
SET(METIS ${CMAKE_CURRENT_SOURCE_DIR}/src/metis)
SET(TAUDEM ${CMAKE_CURRENT_SOURCE_DIR}/src/taudem)
SET(SEIMS_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/src/seims_main)
# SET(METADATA_EXTRACT ${CMAKE_CURRENT_SOURCE_DIR}/src/metadata_extract)
### unit test is an optional configuration.
IF (UNITTEST STREQUAL 1)
    SET(SEIMS_TEST ${CMAKE_CURRENT_SOURCE_DIR}/src/test)
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
    ADD_SUBDIRECTORY(${SEIMS_TEST} bin/test)
ENDIF ()

############  step into subdirectories   #############
ADD_SUBDIRECTORY(${CCGL_LIB} bin/ccgl)
ADD_SUBDIRECTORY(${FIELD_PARTITION} bin/field_partition)
ADD_SUBDIRECTORY(${GRID_LAYERING} bin/grid_layering)
ADD_SUBDIRECTORY(${IUH} bin/iuh)
# ADD_SUBDIRECTORY(${METADATA_EXTRACT} bin/metadata_extract)
ADD_SUBDIRECTORY(${METIS} bin/metis)
IF(BUILD_TAUDEMEXT)
  ADD_SUBDIRECTORY(${TAUDEM} bin/taudem)
ENDIF()
ADD_SUBDIRECTORY(${SEIMS_MAIN} bin/seims_project)

MESSAGE(STATUS "All SEIMS related compiled done!")
