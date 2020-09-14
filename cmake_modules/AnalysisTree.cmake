set(AnalysisTree_INSTALL_DIR ${EXTERNAL_INSTALL_DIR})
set(AnalysisTree_INCLUDE_DIR ${AnalysisTree_INSTALL_DIR}/include)
set(AnalysisTree_LIBRARY_DIR ${AnalysisTree_INSTALL_DIR}/lib)

ExternalProject_Add(AnalysisTree_Ext
        GIT_REPOSITORY  "https://github.com/HeavyIonAnalysis/AnalysisTree.git"
        GIT_TAG         "master"
        UPDATE_DISCONNECTED ${UPDATE_DISCONNECTED}
        SOURCE_DIR      "${EXTERNAL_DIR}/AnalysisTree_src"
        BINARY_DIR      "${EXTERNAL_DIR}/AnalysisTree_build"
        INSTALL_DIR     "${AnalysisTree_INSTALL_DIR}"
        CMAKE_ARGS
        "-DCMAKE_INSTALL_PREFIX=${AnalysisTree_INSTALL_DIR}"
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        "-DROOT_DIR=${ROOT_DIR}"
        "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
        )

# AnalysisTree_INSTALL_DIR will be created during the build stage, not during the stage of configuration
# Due to this CMake won't let me set INTERFACE_INCLUDE_DIRECTORY in the next few lines
# To overcome this internal CMake limitation I'm creating empty include directory
# for the details see https://gitlab.kitware.com/cmake/cmake/-/issues/15052
file(MAKE_DIRECTORY ${AnalysisTree_INSTALL_DIR}/include)

# This is how modern CMake looks like
add_library(AnalysisTreeBase SHARED IMPORTED)
add_dependencies(AnalysisTreeBase AnalysisTree_ext)
set_target_properties(AnalysisTreeBase PROPERTIES IMPORTED_LOCATION ${AnalysisTree_INSTALL_DIR}/lib/libAnalysisTreeBase.so)
set_target_properties(AnalysisTreeBase PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${AnalysisTree_INSTALL_DIR}/include)

add_library(AnalysisTreeInfra SHARED IMPORTED)
add_dependencies(AnalysisTreeInfra  AnalysisTree_ext)
set_target_properties(AnalysisTreeInfra PROPERTIES IMPORTED_LOCATION ${AnalysisTree_INSTALL_DIR}/lib/libAnalysisTreeInfra.so)
set_target_properties(AnalysisTreeInfra PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${AnalysisTree_INSTALL_DIR}/include)

list(APPEND PROJECT_DEPENDENCIES AnalysisTree_Ext)
list(APPEND PROJECT_LINK_DIRECTORIES ${AnalysisTree_LIBRARY_DIR})
list(APPEND PROJECT_INCLUDE_DIRECTORIES ${AnalysisTree_INCLUDE_DIR})