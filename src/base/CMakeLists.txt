set(SOURCES
        Axis.cpp
        Cut.cpp
        QVector.cpp
        AnalysisSetup.cpp
        Variable.cpp
        Histogram.cpp)

string(REPLACE ".cpp" ".h" HEADERS "${SOURCES}")

add_library(QnAnalysisBase SHARED ${SOURCES} G__QnAnalysisBase.cxx)
target_include_directories(QnAnalysisBase PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>/src
        )
message(${PROJECT_LINK_DIRECTORIES})
include_directories(${PROJECT_INCLUDE_DIRECTORIES} ${CMAKE_CURRENT_SOURCE_DIR})
ROOT_GENERATE_DICTIONARY(G__QnAnalysisBase
        ${HEADERS}
        LINKDEF QnAnalysisBaseLinkDef.h
        )
target_link_libraries(QnAnalysisBase
        PUBLIC
            AnalysisTreeBase
            AnalysisTreeInfra
            ROOT::Core
            QnToolsBase
            QnToolsCorrection
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




