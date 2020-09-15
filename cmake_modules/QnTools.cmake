

find_package(QnTools QUIET)

if (NOT QnTools_FOUND)
    include(FetchContent)
    set(QnTools_BUILD_TESTS OFF)
    FetchContent_Declare(QnTools
        GIT_REPOSITORY  git@github.com:eugene274/QnTools.git
        GIT_TAG         issue-33
        UPDATE_DISCONNECTED ${UPDATE_DISCONNECTED}
    )
    FetchContent_MakeAvailable(QnTools)
endif(NOT QnTools_FOUND)
