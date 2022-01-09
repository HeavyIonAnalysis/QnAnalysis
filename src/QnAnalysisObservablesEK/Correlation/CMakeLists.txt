
set(Correlation_LINKDEF ${CMAKE_CURRENT_BINARY_DIR}/CorrelationLinkDef.hpp)
get_filename_component(Correlation_HEADER Correlation.hpp ABSOLUTE)
configure_file(CorrelationLinkDef.hpp.in ${Correlation_LINKDEF})

ROOT_GENERATE_DICTIONARY(G__Correlation
        Correlation.hpp
        LINKDEF ${Correlation_LINKDEF}
        OPTIONS
            -I${CMAKE_CURRENT_SOURCE_DIR}
            -interpreteronly)
add_library(Correlation SHARED
        Correlation.cpp
        G__Correlation.cxx)
target_link_libraries(Correlation PUBLIC
        QnToolsBase
        ROOT::Core
        ROOT::RIO)
target_compile_options(Correlation PRIVATE
        $<$<CONFIG:DEBUG>:-g -ggdb -O0>
        $<$<CONFIG:RELEASE>:-O2>)
target_include_directories(Correlation
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(CorrelationTests Correlation.test.cpp)
target_link_libraries(CorrelationTests PRIVATE Correlation gtest_main)
