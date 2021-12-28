
add_library(Correlation Correlation.cpp)
target_link_libraries(Correlation PUBLIC QnToolsBase ROOT::Core ROOT::RIO)
target_compile_options(Correlation PUBLIC -g -ggdb -O0)

add_executable(CorrelationTests Correlation.test.cpp)
target_link_libraries(CorrelationTests PRIVATE Correlation gtest_main)