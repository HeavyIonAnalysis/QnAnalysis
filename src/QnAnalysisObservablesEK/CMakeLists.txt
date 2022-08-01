
find_package(Boost REQUIRED COMPONENTS regex)

add_subdirectory(gse)


ROOT_GENERATE_DICTIONARY(
        G__QnToolsExtension
        ${CMAKE_CURRENT_SOURCE_DIR}/QnSystematicError.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/QnDv1Dy.hpp
        LINKDEF QnToolsExtensionLinkDef.h
        OPTIONS -I${CMAKE_CURRENT_SOURCE_DIR}/gse)
add_library(QnToolsExtension SHARED QnSystematicError.cpp QnDv1Dy.cpp G__QnToolsExtension.cxx)
target_link_libraries(QnToolsExtension PUBLIC
        ${ROOT_LIBRARIES}
        QnTools::Base
        gse
        )

add_library(QnAnalysisObservables INTERFACE)
target_include_directories(QnAnalysisObservables INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(QnAnalysisObservables INTERFACE
        QnTools::Base
        QnToolsExtension
        QnAnalysisTools
        gse
        ${ROOT_LIBRARIES}
        ${Boost_LIBRARIES}
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>)

add_subdirectory(na61)
