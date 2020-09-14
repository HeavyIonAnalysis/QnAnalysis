set(QnTools_INSTALL_DIR ${EXTERNAL_INSTALL_DIR})
set(QnTools_INCLUDE_DIR ${QnTools_INSTALL_DIR}/include)
set(QnTools_LIBRARY_DIR ${QnTools_INSTALL_DIR}/lib)

ExternalProject_Add(QnTools_Ext
        GIT_REPOSITORY  "https://github.com/HeavyIonAnalysis/QnTools.git"
        GIT_TAG         "defca4cc9f7876e854f1af5f7c622255dbc4b508"
        UPDATE_DISCONNECTED ${UPDATE_DISCONNECTED}
        SOURCE_DIR      "${EXTERNAL_DIR}/QnTools_src"
        BINARY_DIR      "${EXTERNAL_DIR}/QnTools_build"
        INSTALL_DIR     "${QnTools_INSTALL_DIR}"
        CMAKE_ARGS
            "-DCMAKE_INSTALL_PREFIX=${QnTools_INSTALL_DIR}"
            "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
            "-DROOT_DIR=${ROOT_DIR}"
            "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
)

# QnTools_INSTALL_DIR will be created during the build stage, not during the stage of configuration
# Due to this CMake won't let me set INTERFACE_INCLUDE_DIRECTORY in the next few lines
# To overcome this internal CMake limitation I'm creating empty include directory
# for the details see https://gitlab.kitware.com/cmake/cmake/-/issues/15052
file(MAKE_DIRECTORY ${QnTools_INSTALL_DIR}/include)

# This is how modern CMake looks like
add_library(QnToolsBase SHARED IMPORTED)
add_dependencies(QnToolsBase QnTools_ext)
set_target_properties(QnToolsBase PROPERTIES IMPORTED_LOCATION ${QnTools_INSTALL_DIR}/lib/libQnToolsBase.so)
set_target_properties(QnToolsBase PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${QnTools_INSTALL_DIR}/include)

add_library(QnToolsCorrection SHARED IMPORTED)
add_dependencies(QnToolsCorrection  QnTools_ext)
set_target_properties(QnToolsCorrection PROPERTIES IMPORTED_LOCATION ${QnTools_INSTALL_DIR}/lib/libQnToolsCorrection.so)
set_target_properties(QnToolsCorrection PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${QnTools_INSTALL_DIR}/include)

list(APPEND PROJECT_DEPENDENCIES QnTools_Ext)
list(APPEND PROJECT_LINK_DIRECTORIES ${QnTools_LIBRARY_DIR})
list(APPEND PROJECT_INCLUDE_DIRECTORIES ${QnTools_INCLUDE_DIR})