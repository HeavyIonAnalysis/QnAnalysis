cmake_minimum_required(VERSION 3.12)
project(QnAnalysis CXX)
set(CMAKE_CXX_STANDARD 17)
set(PROJECT_VERSION 1.0)

include(ExternalProject)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
message(STATUS "Using C++${CMAKE_CXX_STANDARD}")

option(yaml-cpp_BUNDLED "Build bundled yaml-cpp" OFF)
option(QnAnalysis_BUILD_TESTS "Build tests" ON)

# by default build optimized code with debug symbols
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RELEASE)
endif ()

# in DEBUG mode make verbose Makefile
if (CMAKE_BUILD_TYPE MATCHES DEBUG)
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif ()


set(EXTERNAL_DIR ${CMAKE_BINARY_DIR}/external)
set(EXTERNAL_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/external)

set(QnAnalysis_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

include(cmake_modules/YamlCpp.cmake)
include(cmake_modules/Root.cmake)
#include(cmake_modules/Cuts.cmake)
include(cmake_modules/QnTools.cmake)
include(cmake_modules/AnalysisTree.cmake)
include(cmake_modules/AnalysisTreeTaskSkeleton.cmake)


set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb -g -DDEBUG -D__DEBUG -Wall")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ftree-vectorize -ffast-math -DNODEBUG")
message(STATUS "Using CXX flags for ${CMAKE_BUILD_TYPE}: ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")

SET(QN_PATH             "" CACHE STRING "Path to QnAnalysis")
list(APPEND CMAKE_PREFIX_PATH ${QN_PATH})
list(APPEND CMAKE_PREFIX_PATH $ENV{QN_PATH})

include_directories(${CMAKE_SOURCE_DIR}/src/base ${AnalysisTree_INCLUDE_DIR} ${PROJECT_INCLUDE_DIRECTORIES})
add_subdirectory(src)
add_subdirectory(test)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/QnAnalysis/QnAnalysisConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)

export(EXPORT QnAnalysisTargets
        FILE "${CMAKE_CURRENT_BINARY_DIR}/QnAnalysis/QnAnalysisTargets.cmake")

message(STATUS "QnAnalysisTargets: ${QnAnalysisTargets}")
        
set(ConfigPackageLocation lib/cmake/QnAnalysis)

set(QnAnalysis_INCLUDE_DIR "include")
set(QnAnalysis_LIBRARY_DIR "lib")

configure_package_config_file(
        QnAnalysisConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/QnAnalysis/QnAnalysisConfig.cmake
        INSTALL_DESTINATION ${ConfigPackageLocation}
        PATH_VARS QnAnalysis_INCLUDE_DIR QnAnalysis_LIBRARY_DIR
)

install(EXPORT QnAnalysisTargets
        FILE
        QnAnalysisTargets.cmake
        DESTINATION
        ${ConfigPackageLocation}
        )

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/QnAnalysis/QnAnalysisConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/QnAnalysis/QnAnalysisConfigVersion.cmake"
        DESTINATION
        ${ConfigPackageLocation}
        COMPONENT
        Devel
        )
