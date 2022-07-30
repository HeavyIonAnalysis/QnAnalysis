set(SOURCES
        Axis.cpp
        Cut.cpp
        QVector.cpp
        AnalysisSetup.cpp
        Variable.cpp
        Histogram.cpp)
        
link_directories(${QnTools_LIBS_DIR})

add_library(QnAnalysisBase SHARED ${SOURCES} G__QnAnalysisBase.cxx)
target_include_directories(QnAnalysisBase PUBLIC $<BUILD_INTERFACE:${QnAnalysis_SOURCE_DIR}>)
include_directories(${PROJECT_INCLUDE_DIRECTORIES} ${QnAnalysis_SOURCE_DIR} ${AnalysisTree_BINARY_DIR}/include)

string(REPLACE ".cpp" ".hpp" HEADERS "${SOURCES}")
list(TRANSFORM HEADERS PREPEND QnAnalysisBase/)
ROOT_GENERATE_DICTIONARY(G__QnAnalysisBase ${HEADERS} 
    LINKDEF QnAnalysisBaseLinkDef.hpp
    OPTIONS -I${QnAnalysis_SOURCE_DIR}
    OPTIONS -I${AnalysisTree_BINARY_DIR}/include
    )

list(TRANSFORM HEADERS REPLACE "QnAnalysisBase/" "")

target_link_libraries(QnAnalysisBase
        PUBLIC
        AnalysisTreeInfra
        $<TARGET_NAME_IF_EXISTS:AnalysisTreeInfraVersion1>
        AnalysisTreeBase
        QnTools::Base
        QnTools::Correction
        ROOT::Core
        )

install(TARGETS QnAnalysisBase EXPORT QnAnalysisTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )

install(
        FILES
        ${HEADERS}
        DESTINATION
        include/QnAnalysisBase
        COMPONENT
        Devel
)

set(PCM_FILE_NAME libQnAnalysisBase)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PCM_FILE_NAME}_rdict.pcm"
        DESTINATION
        lib
        OPTIONAL
)




