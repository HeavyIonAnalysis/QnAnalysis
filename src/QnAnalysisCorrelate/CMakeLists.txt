
set(QnAnalysis_SETUPS_DIR ${CMAKE_SOURCE_DIR}/setups)

configure_file(BuildOptions.hpp.in BuildOptions.hpp)

add_executable(QnAnalysisCorrelate Correlation.cpp CorrelationTask.cpp)
target_link_libraries(QnAnalysisCorrelate PUBLIC QnToolsDataFrame)

if (QnAnalysis_BUILD_TESTS)
    include(GoogleTest)
    add_executable(QnAnalysisCorrelate_UnitTests Config.test.cpp)
    target_link_libraries(QnAnalysisCorrelate_UnitTests PRIVATE gtest_main yaml-cpp)
    target_include_directories(QnAnalysisCorrelate_UnitTests PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    gtest_add_tests(TARGET QnAnalysisCorrelate_UnitTests)
endif ()