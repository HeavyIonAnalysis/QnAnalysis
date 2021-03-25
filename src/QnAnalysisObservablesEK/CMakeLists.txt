
find_package(Boost REQUIRED COMPONENTS regex)

add_executable(QnAnalysisObservables na61.cpp)
target_link_libraries(QnAnalysisObservables PUBLIC
        QnToolsBase ${ROOT_LIBRARIES} ${Boost_LIBRARIES}
        QnAnalysisTools
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
        )