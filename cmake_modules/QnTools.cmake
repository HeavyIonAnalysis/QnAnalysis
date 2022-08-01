

if (QNTOOLS_ROOT)
    message("-- Uing external QnTools from ${QNTOOLS_ROOT}")
    set(CMAKE_PREFIX_PATH ${QNTOOLS_ROOT} ${CMAKE_PREFIX_PATH})
    find_package(QnTools)
    list(APPEND PROJECT_INCLUDE_DIRECTORIES ${QnTools_INCLUDE_DIR})
else()

    include(FetchContent)
    FetchContent_Declare(QnTools
            GIT_REPOSITORY https://github.com/HeavyIonAnalysis/QnTools.git
            GIT_TAG "7789d40de12a8936fb0bac67ad7313c345177a20"
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

endif(QNTOOLS_ROOT)
