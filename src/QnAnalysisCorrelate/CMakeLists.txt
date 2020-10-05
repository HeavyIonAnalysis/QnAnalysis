
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(yaml-cpp REQUIRED)

set(QnAnalysis_SETUPS_DIR ${CMAKE_SOURCE_DIR}/setups)

configure_file(BuildOptions.hpp.in BuildOptions.hpp)

add_executable(QnAnalysisCorrelate CorrelationMain.cpp CorrelationTaskRunner.cpp)
target_link_libraries(QnAnalysisCorrelate PUBLIC QnToolsDataFrame Boost::program_options yaml-cpp)
target_include_directories(QnAnalysisCorrelate PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if (QnAnalysis_BUILD_TESTS)
    include(GoogleTest)
    add_executable(QnAnalysisCorrelate_UnitTests Config.test.cpp Utils.test.cpp)
    target_link_libraries(QnAnalysisCorrelate_UnitTests PRIVATE gtest_main yaml-cpp)
    target_include_directories(QnAnalysisCorrelate_UnitTests PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    gtest_add_tests(TARGET QnAnalysisCorrelate_UnitTests)
endif ()