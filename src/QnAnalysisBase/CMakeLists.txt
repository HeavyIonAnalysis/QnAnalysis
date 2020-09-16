set(SOURCES
        Axis.cpp
        Cut.cpp
        QVector.cpp
        AnalysisSetup.cpp
        Variable.cpp
        Histogram.cpp)


add_library(QnAnalysisBase SHARED ${SOURCES} G__QnAnalysisBase.cxx)
target_include_directories(QnAnalysisBase PUBLIC $<BUILD_INTERFACE:${QnAnalysis_SOURCE_DIR}>)
include_directories(${PROJECT_INCLUDE_DIRECTORIES} ${QnAnalysis_SOURCE_DIR})

string(REPLACE ".cpp" ".hpp" HEADERS "${SOURCES}")
list(TRANSFORM HEADERS PREPEND QnAnalysisBase/)
ROOT_GENERATE_DICTIONARY(G__QnAnalysisBase ${HEADERS} LINKDEF QnAnalysisBaseLinkDef.hpp)

target_link_libraries(QnAnalysisBase
        PUBLIC
        AnalysisTreeInfra
        AnalysisTreeBase
        QnToolsBase
        QnToolsCorrection
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
        include
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




