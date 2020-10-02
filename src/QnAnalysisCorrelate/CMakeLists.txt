

add_executable(QnAnalysisCorrelate Correlation.cpp CorrelationTask.cpp)
target_link_libraries(QnAnalysisCorrelate PUBLIC QnToolsDataFrame)

if (QnAnalysis_BUILD_TESTS)
    include(GoogleTest)
    add_executable(QnAnalysisCorrelate_UnitTests Config.test.cpp)
    target_link_libraries(QnAnalysisCorrelate_UnitTests PRIVATE gtest_main yaml-cpp)
    gtest_add_tests(TARGET QnAnalysisCorrelate_UnitTests)
endif ()