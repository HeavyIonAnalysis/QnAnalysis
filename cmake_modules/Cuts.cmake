#
# Cuts
#

set(Cuts_INSTALL_DIR ${EXTERNAL_INSTALL_DIR}/)
set(Cuts_INCLUDE_DIR ${Cuts_INSTALL_DIR}/include)
set(Cuts_LIBRARY_DIR ${Cuts_INSTALL_DIR}/lib)

ExternalProject_Add(Cuts_Ext
        GIT_REPOSITORY  "https://git.cbm.gsi.de/pwg-c2f/analysis/cuts.git"
        GIT_TAG         "at-v2"
        UPDATE_DISCONNECTED ${UPDATE_DISCONNECTED}
        SOURCE_DIR      "${EXTERNAL_DIR}/Cuts_src"
        BINARY_DIR      "${EXTERNAL_DIR}/Cuts_build"
        INSTALL_DIR     "${Cuts_INSTALL_DIR}"
        CMAKE_ARGS
        "-DCMAKE_INSTALL_PREFIX=${Cuts_INSTALL_DIR}"
        "-DROOT_DIR=${ROOT_DIR}"
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        )

list(APPEND PROJECT_DEPENDENCIES Cuts_Ext)
list(APPEND PROJECT_LINK_DIRECTORIES ${Cuts_LIBRARY_DIR})
list(APPEND PROJECT_INCLUDE_DIRECTORIES ${Cuts_INCLUDE_DIR})
