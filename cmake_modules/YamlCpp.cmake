if (yaml-cpp_BUNDLED)
    message("-- Building bundled yaml-cpp package...")
    include(ExternalProject)

    ExternalProject_Add(
            yaml-cpp_Ext
            GIT_REPOSITORY "https://github.com/jbeder/yaml-cpp.git"
            GIT_TAG     yaml-cpp-0.6.3
            GIT_SHALLOW ON
            CMAKE_ARGS
                -DYAML_BUILD_SHARED_LIBS=ON
                -DCMAKE_BUILD_TYPE=Release
                -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/external/yaml-cpp
            PREFIX ${CMAKE_BINARY_DIR}/external/yaml-cpp
    )



    add_library(yaml-cpp SHARED IMPORTED GLOBAL)
    set_target_properties(yaml-cpp PROPERTIES
            IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/external/yaml-cpp/lib/libyaml-cpp.so
            INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR}/external/yaml-cpp/include
            )
    add_dependencies(yaml-cpp yaml-cpp_Ext)

    set(yaml-cpp_FOUND TRUE)
    set(yaml-cpp_INCLUDE_DIR ${CMAKE_BINARY_DIR}/external/yaml-cpp/include)
else()
    find_package(yaml-cpp REQUIRED)
endif ()