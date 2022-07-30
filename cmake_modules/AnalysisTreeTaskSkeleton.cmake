FetchContent_Declare(ATTaskSkeleton
        GIT_REPOSITORY https://github.com/lubynets/AnalysisTreeTaskSkeleton.git
        GIT_TAG "at_cuts_hash"
        GIT_SHALLOW ON
        UPDATE_DISCONNECTED ${UPDATE_DISCONNECTED}
        )

FetchContent_GetProperties(ATTaskSkeleton)
if(NOT attaskskeleton_POPULATED)
    FetchContent_Populate(ATTaskSkeleton)
    add_subdirectory(${attaskskeleton_SOURCE_DIR} ${attaskskeleton_BINARY_DIR})
endif()
