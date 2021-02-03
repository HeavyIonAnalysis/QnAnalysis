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


if (QnAnalysis_BUILD_TESTS)
    include(GoogleTest)
    add_executable(QnAnalysisConfig_UnitTests YamlUtils.test.cpp)
    target_link_libraries(QnAnalysisConfig_UnitTests PRIVATE gtest_main yaml-cpp)
    gtest_add_tests(TARGET QnAnalysisConfig_UnitTests)
endif (QnAnalysis_BUILD_TESTS)