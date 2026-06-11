find_package(QnTools QUIET)
if(NOT QnTools_FOUND)
    message("-- QnTools not found, will be built automatically")

    set(QnAnalysis_BUNDLED_QNT_URL "https://github.com/HeavyIonAnalysis/QnTools.git" CACHE STRING "Bundled QnTools URL")
    set(QnAnalysis_BUNDLED_QNT_VERSION "7789d40de12a8936fb0bac67ad7313c345177a20" CACHE STRING "Bundled QnTools version")
    set(QnAnalysis_BUNDLED_QNT_GIT_SHALLOW ON CACHE BOOL "Use CMake GIT_SHALLOW option")

    include(FetchContent)
    FetchContent_Declare(QnTools
        GIT_REPOSITORY  ${QnAnalysis_BUNDLED_QNT_URL}
        GIT_TAG         ${QnAnalysis_BUNDLED_QNT_VERSION}
        GIT_SHALLOW     ${QnAnalysis_BUNDLED_QNT_GIT_SHALLOW}
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
else()
    message("-- QnTools found")
    list(APPEND PROJECT_INCLUDE_DIRECTORIES ${QnTools_INCLUDE_DIR})
endif()
