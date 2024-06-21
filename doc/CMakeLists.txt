# Modified from RapidJSON
# https://github.com/Tencent/rapidjson/blob/master/doc/CMakeLists.txt
# commit/bbdf5d1d4b40891c82e5c1946d32dfc841926066 Sep 6, 2017
#
find_package(Doxygen)

IF(NOT DOXYGEN_FOUND)
    MESSAGE(STATUS "No Doxygen found. Documentation won't be built")
ELSE()
    file(GLOB SOURCES ${CMAKE_CURRENT_LIST_DIR}/../seims/*
                      )
    file(GLOB MARKDOWN_DOC ${CMAKE_CURRENT_LIST_DIR}/en/*.md
                           ${CMAKE_CURRENT_LIST_DIR}/zh-cn/*.md
                           ${CMAKE_CURRENT_LIST_DIR}/common/*.md)
    list(APPEND MARKDOWN_DOC ${CMAKE_CURRENT_LIST_DIR}/../README.md
                             ${CMAKE_CURRENT_LIST_DIR}/../README.zh-cn.md)

    CONFIGURE_FILE(Doxyfile.in Doxyfile @ONLY)
    CONFIGURE_FILE(Doxyfile.zh-cn.in Doxyfile.zh-cn @ONLY)

    file(GLOB DOXYFILES ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile*)

    # status(${CMAKE_CURRENT_SOURCE_DIR}) # e.g., SEIMS/doc
    # status(${CMAKE_CURRENT_LIST_DIR}) # e.g., SEIMS/doc
    # status(${CMAKE_CURRENT_BINARY_DIR}) # SEIMS/cmake-build-debug-gcc/doc

    add_custom_command(OUTPUT html
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.zh-cn
        DEPENDS ${MARKDOWN_DOC} ${SOURCES} ${DOXYFILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../
        )

    add_custom_command(OUTPUT copyusermanual
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/../SEIMS-UserManual.pdf
        COMMENT "Copying UserManual PDF"
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/../SEIMS-UserManual.pdf html/SEIMS-UserManual.pdf
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/../SEIMS-UserManual.pdf html/zh-cn/SEIMS-UserManual.pdf
        )

    add_custom_target(seimsdoc ALL 
        DEPENDS html copyusermanual
        )
ENDIF()
