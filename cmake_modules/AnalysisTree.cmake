
find_package(AnalysisTree QUIET)


if(NOT AnalysisTree_FOUND)
set(AnalysisTree_BUILD_EXAMPLES OFF)

include(FetchContent)
FetchContent_Declare(AnalysisTree
        GIT_REPOSITORY  "https://github.com/HeavyIonAnalysis/AnalysisTree.git"
        GIT_TAG         "master"
        UPDATE_DISCONNECTED ${UPDATE_DISCONNECTED}
        )
FetchContent_MakeAvailable(AnalysisTree)
endif(NOT AnalysisTree_FOUND)


get_target_property(AnalysisTreeBase_INCLUDE_DIR AnalysisTreeBase INCLUDE_DIRECTORIES)
list(APPEND PROJECT_INCLUDE_DIRECTORIES ${AnalysisTreeBase_INCLUDE_DIR})
get_target_property(AnalysisTreeInfra_INCLUDE_DIR AnalysisTreeInfra INCLUDE_DIRECTORIES)
list(APPEND PROJECT_INCLUDE_DIRECTORIES ${AnalysisTreeInfra_INCLUDE_DIR})
