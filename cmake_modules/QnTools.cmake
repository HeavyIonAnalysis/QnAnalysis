

find_package(QnTools QUIET)

if (NOT QnTools_FOUND)
    include(FetchContent)
    set(QnTools_BUILD_TESTS OFF)
    FetchContent_Declare(QnTools
        GIT_REPOSITORY  https://github.com/eugene274/QnTools.git
        GIT_TAG         issue-33
        UPDATE_DISCONNECTED ${UPDATE_DISCONNECTED}
    )
    FetchContent_MakeAvailable(QnTools)
endif(NOT QnTools_FOUND)

get_target_property(QnToolsBase_INCLUDE_DIR QnToolsBase INCLUDE_DIRECTORIES)
get_target_property(QnToolsCorrection_INCLUDE_DIR QnToolsCorrection INCLUDE_DIRECTORIES)

list(APPEND PROJECT_INCLUDE_DIRECTORIES ${QnToolsBase_INCLUDE_DIR})
list(APPEND PROJECT_INCLUDE_DIRECTORIES ${QnToolsCorrection_INCLUDE_DIR})
