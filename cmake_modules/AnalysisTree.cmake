find_package(AnalysisTree QUIET)
if(NOT AnalysisTree_FOUND)
message("-- AnalysisTree not found, will be built automatically")

set(AnalysisTree_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(AnalysisTree_BUILD_INFRA_1 ON CACHE INTERNAL "")
set(QnAnalysis_BUNDLED_AT_VERSION "v2.3.2" CACHE STRING "Bundled AnalysisTree version")
set(QnAnalysis_BUNDLED_AT_GIT_SHALLOW ON CACHE BOOL "Use CMake GIT_SHALLOW option")

include(FetchContent)
FetchContent_Declare(AnalysisTree
        GIT_REPOSITORY  "https://github.com/HeavyIonAnalysis/AnalysisTree.git"
        GIT_TAG         ${QnAnalysis_BUNDLED_AT_VERSION}
        GIT_SHALLOW     ${QnAnalysis_BUNDLED_AT_GIT_SHALLOW}
        UPDATE_DISCONNECTED ${UPDATE_DISCONNECTED}
        )

FetchContent_GetProperties(AnalysisTree)
if(NOT analysistree_POPULATED)
    FetchContent_Populate(AnalysisTree)
    add_subdirectory(${analysistree_SOURCE_DIR} ${analysistree_BINARY_DIR})
endif()

get_target_property(AnalysisTreeBase_INCLUDE_DIR AnalysisTreeBase INCLUDE_DIRECTORIES)
list(APPEND PROJECT_INCLUDE_DIRECTORIES ${AnalysisTreeBase_INCLUDE_DIR})
get_target_property(AnalysisTreeInfra_INCLUDE_DIR AnalysisTreeInfra INCLUDE_DIRECTORIES)
list(APPEND PROJECT_INCLUDE_DIRECTORIES ${AnalysisTreeInfra_INCLUDE_DIR})

else()
message("-- AnalysisTree found")
endif()
