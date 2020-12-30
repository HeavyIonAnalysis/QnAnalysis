

find_package(QnTools QUIET)

include(FetchContent)
FetchContent_Declare(QnTools
        GIT_REPOSITORY https://github.com/HeavyIonAnalysis/QnTools.git
        GIT_TAG "f39fe1fd34d9b01037e28e369a25c9fe03f0b4e4" # PR: Fix bootstrapping error propagation of scaling and pow operations
        UPDATE_DISCONNECTED ${UPDATE_DISCONNECTED}
        )

FetchContent_GetProperties(QnTools)
if(NOT qntools_POPULATED)
    FetchContent_Populate(QnTools)
    set(QnTools_BUILD_TESTS OFF)
    add_subdirectory(${qntools_SOURCE_DIR} ${qntools_BINARY_DIR})
endif()

get_target_property(QnToolsBase_INCLUDE_DIRS QnToolsBase INCLUDE_DIRECTORIES)
list(APPEND PROJECT_INCLUDE_DIRECTORIES ${QnToolsBase_INCLUDE_DIRS})
get_target_property(QnToolsCorrection_INCLUDE_DIRS QnToolsCorrection INCLUDE_DIRECTORIES)
list(APPEND PROJECT_INCLUDE_DIRECTORIES ${QnToolsCorrection_INCLUDE_DIRS})
