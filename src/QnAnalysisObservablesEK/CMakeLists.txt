
find_package(Boost REQUIRED COMPONENTS regex)

ROOT_GENERATE_DICTIONARY(
        G__QnToolsExtension
        ${CMAKE_CURRENT_SOURCE_DIR}/QnSystematicError.hpp
        LINKDEF QnToolsExtensionLinkDef.h)
add_library(QnToolsExtension SHARED QnSystematicError.cpp G__QnToolsExtension.cxx)


add_executable(QnAnalysisObservables na61.cpp )
target_link_libraries(QnAnalysisObservables PUBLIC
        QnToolsBase
        QnToolsExtension
        ${ROOT_LIBRARIES}
        ${Boost_LIBRARIES}
        QnAnalysisTools
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
        )