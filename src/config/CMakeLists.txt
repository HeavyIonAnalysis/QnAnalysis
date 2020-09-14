find_package(yaml-cpp REQUIRED)

add_library(FlowConfig STATIC Convert.cpp Config.cpp)
target_include_directories(FlowConfig PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>/src>
        )
target_link_libraries(FlowConfig PUBLIC
        FlowBase
        QnToolsCorrection
        yaml-cpp
        )

add_executable(ReadConfig ReadConfig.cpp)
target_link_libraries(ReadConfig PRIVATE FlowConfig)