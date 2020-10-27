
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(yaml-cpp REQUIRED)

set(QnAnalysis_SETUPS_DIR ${CMAKE_SOURCE_DIR}/setups)

configure_file(BuildOptions.hpp.in BuildOptions.hpp)

ROOT_GENERATE_DICTIONARY(G__TStringMeta
        TStringMeta.hpp
        LINKDEF TStringMetaLinkDef.h
        OPTIONS -I${CMAKE_CURRENT_SOURCE_DIR})
add_library(TStringMeta SHARED TStringMeta.cpp G__TStringMeta.cxx)
target_link_libraries(TStringMeta PUBLIC ROOT::Core)
target_include_directories(TStringMeta PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(QnAnalysisCorrelate CorrelationMain.cpp CorrelationTaskRunner.cpp)
target_link_libraries(QnAnalysisCorrelate
        PRIVATE
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
        PUBLIC
        QnToolsDataFrame Boost::program_options yaml-cpp TStringMeta)
target_include_directories(QnAnalysisCorrelate PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if (QnAnalysis_BUILD_TESTS)
    include(GoogleTest)
    add_executable(QnAnalysisCorrelate_UnitTests Config.test.cpp Utils.test.cpp UserCorrelationAction.test.cpp TStringMeta.test.cpp)
    target_link_libraries(QnAnalysisCorrelate_UnitTests PRIVATE gtest_main yaml-cpp QnToolsDataFrame TStringMeta)
    target_include_directories(QnAnalysisCorrelate_UnitTests PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    gtest_add_tests(TARGET QnAnalysisCorrelate_UnitTests)
endif ()