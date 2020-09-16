

find_package(QnTools QUIET)

include(FetchContent)
set(QnTools_BUILD_TESTS OFF)
FetchContent_Declare(QnTools
        GIT_REPOSITORY https://github.com/eugene274/QnTools.git
        GIT_TAG issue-33
        UPDATE_DISCONNECTED ${UPDATE_DISCONNECTED}
        )

FetchContent_GetProperties(QnTools)
if(NOT qntools_POPULATED)
    FetchContent_Populate(QnTools)
    add_subdirectory(${qntools_SOURCE_DIR} ${qntools_BINARY_DIR})
endif()

list(APPEND PROJECT_INCLUDE_DIRECTORIES ${qntools_SOURCE_DIR}/include/base)
list(APPEND PROJECT_INCLUDE_DIRECTORIES ${qntools_SOURCE_DIR}/include/correction)
