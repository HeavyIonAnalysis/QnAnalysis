set(Flow_INSTALL_DIR ${EXTERNAL_INSTALL_DIR})
set(Flow_INCLUDE_DIR ${Flow_INSTALL_DIR}/include)
set(Flow_LIBRARY_DIR ${Flow_INSTALL_DIR}/lib)

ExternalProject_Add(Flow_Ext
        GIT_REPOSITORY  "https://github.com/HeavyIonAnalysis/QnTools.git"
        GIT_TAG         "defca4cc9f7876e854f1af5f7c622255dbc4b508"
        UPDATE_DISCONNECTED ${UPDATE_DISCONNECTED}
        SOURCE_DIR      "${EXTERNAL_DIR}/Flow_src"
        BINARY_DIR      "${EXTERNAL_DIR}/Flow_build"
        INSTALL_DIR     "${Flow_INSTALL_DIR}"
        CMAKE_ARGS
            "-DCMAKE_INSTALL_PREFIX=${Flow_INSTALL_DIR}"
            "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
            "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
)

# Flow_INSTALL_DIR will be created during the build stage, not during the stage of configuration
# Due to this CMake won't let me set INTERFACE_INCLUDE_DIRECTORY in the next few lines
# To overcome this internal CMake limitation I'm creating empty include directory
# for the details see https://gitlab.kitware.com/cmake/cmake/-/issues/15052
file(MAKE_DIRECTORY ${Flow_INSTALL_DIR}/include)

# This is how modern CMake looks like
add_library(QnToolsBase SHARED IMPORTED)
add_dependencies(QnToolsBase Flow_ext)
set_target_properties(QnToolsBase PROPERTIES IMPORTED_LOCATION ${Flow_INSTALL_DIR}/lib/libQnToolsBase.so)
set_target_properties(QnToolsBase PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${Flow_INSTALL_DIR}/include)

add_library(QnToolsCorrection SHARED IMPORTED)
add_dependencies(QnToolsCorrection  Flow_ext)
set_target_properties(QnToolsCorrection PROPERTIES IMPORTED_LOCATION ${Flow_INSTALL_DIR}/lib/libQnToolsCorrection.so)
set_target_properties(QnToolsCorrection PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${Flow_INSTALL_DIR}/include)

list(APPEND PROJECT_DEPENDENCIES Flow_Ext)
list(APPEND PROJECT_LINK_DIRECTORIES ${Flow_LIBRARY_DIR})
list(APPEND PROJECT_INCLUDE_DIRECTORIES ${Flow_INCLUDE_DIR})