find_package(yaml-cpp QUIET)

if(yaml-cpp_FOUND)
    Message("yaml-cpp found")
Else()
    Message("yaml-cpp is not found. It will be installed as external package")
    include(FetchContent)
    FetchContent_Declare(
            yaml-cpp
            GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
            GIT_SHALLOW	ON
            GIT_TAG yaml-cpp-0.6.3
    )
    FetchContent_GetProperties(yaml-cpp)
    if(NOT yaml-cpp_POPULATED)
        message(STATUS "Populating yaml-cpp...")
        FetchContent_Populate(yaml-cpp)
        # Add here options for yaml-cpp building
        set(YAML_CPP_BUILD_TESTS OFF)
        add_subdirectory(${yaml-cpp_SOURCE_DIR} ${yaml-cpp_BINARY_DIR})
        message(STATUS "Done.")
    endif()
EndIf()

add_library(QnAnalysisConfig STATIC Convert.cpp Config.cpp)
target_include_directories(QnAnalysisConfig PUBLIC
        $<BUILD_INTERFACE:${QnAnalysis_SOURCE_DIR}>>
        )
target_link_libraries(QnAnalysisConfig PUBLIC
        QnAnalysisBase
        yaml-cpp
        )

add_executable(ReadConfig ReadConfig.cpp)
target_link_libraries(ReadConfig PRIVATE QnAnalysisConfig)