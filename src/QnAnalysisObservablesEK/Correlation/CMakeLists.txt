
ROOT_GENERATE_DICTIONARY(G__Correlation
        Correlation.hpp
        LINKDEF CorrelationLinkDef.hpp
        OPTIONS -I${CMAKE_CURRENT_SOURCE_DIR})
add_library(Correlation SHARED
        Correlation.cpp
        G__Correlation.cxx)
target_link_libraries(Correlation PUBLIC
        QnToolsBase
        ROOT::Core
        ROOT::RIO)
target_compile_options(Correlation PUBLIC -g -ggdb -O0)
target_include_directories(Correlation
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(CorrelationTests Correlation.test.cpp)
target_link_libraries(CorrelationTests PRIVATE Correlation gtest_main)
