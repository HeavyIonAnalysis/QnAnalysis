
set(CMAKE_CXX_STANDARD 17)
link_directories(${QnTools_LIBS_DIR})

try_compile(
        HAS_STD_FILESYSTEM ${CMAKE_BINARY_DIR}
        SOURCES ${CMAKE_SOURCE_DIR}/cmake_tests_src/test_filesystem.cxx
        LINK_LIBRARIES $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
)
if (HAS_STD_FILESYSTEM)
    message("-- Found std::filesystem")
else()
    find_package(Boost REQUIRED COMPONENTS filesystem)
    set(HAS_BOOST_FILESYSTEM ${Boost_FILESYSTEM_FOUND})
    message("-- Found boost::filesystem")
endif()

find_package(Boost REQUIRED COMPONENTS program_options)

set(QnAnalysis_SETUPS_DIR ${CMAKE_SOURCE_DIR}/setups)

configure_file(BuildOptions.hpp.in BuildOptions.hpp)

include_directories(${QnTools_INCLUDE_DIR}/QnTools)

add_executable(QnAnalysisCorrelate CorrelationMain.cpp CorrelationTaskRunner.cpp)
target_link_libraries(QnAnalysisCorrelate
        PRIVATE
            # link std::filesystem if compiler supports it
            $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
            # link Boost::filesystem if it was found
            $<$<BOOL:${HAS_BOOST_FILESYSTEM}>:Boost::filesystem>
        PUBLIC
        QnTools::DataFrame Boost::program_options yaml-cpp)
target_compile_definitions(QnAnalysisCorrelate
        PRIVATE
            $<$<BOOL:${HAS_STD_FILESYSTEM}>:HAS_STD_FILESYSTEM>
            $<$<BOOL:${HAS_BOOST_FILESYSTEM}>:HAS_BOOST_FILESYSTEM>
        )
target_include_directories(QnAnalysisCorrelate PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if (QnAnalysis_BUILD_TESTS)
    include(GoogleTest)
    add_executable(QnAnalysisCorrelate_UnitTests Config.test.cpp Utils.test.cpp)
    target_link_libraries(QnAnalysisCorrelate_UnitTests PRIVATE gtest_main yaml-cpp QnTools::DataFrame)
    target_include_directories(QnAnalysisCorrelate_UnitTests PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    gtest_add_tests(TARGET QnAnalysisCorrelate_UnitTests)
endif ()

install(TARGETS QnAnalysisCorrelate EXPORT QnAnalysisCorrelateTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )

set(PCM_FILE_NAME libQnAnalysisCorrelate)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PCM_FILE_NAME}_rdict.pcm"
        DESTINATION
        lib
        OPTIONAL
)