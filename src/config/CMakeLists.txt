find_package(yaml-cpp REQUIRED)

add_library(QnAnalysisConfig STATIC Convert.cpp Config.cpp)
target_include_directories(QnAnalysisConfig PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>/src>
        )
target_link_libraries(QnAnalysisConfig PUBLIC
        QnAnalysisBase
        QnToolsCorrection
        yaml-cpp
        )

add_executable(ReadConfig ReadConfig.cpp)
target_link_libraries(ReadConfig PRIVATE QnAnalysisConfig)