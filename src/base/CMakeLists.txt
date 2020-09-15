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
link_directories(${PROJECT_LINK_DIRECTORIES})
add_dependencies(QnAnalysisBase ${PROJECT_DEPENDENCIES})
include_directories(${PROJECT_INCLUDE_DIRECTORIES} ${CMAKE_CURRENT_SOURCE_DIR})


# TODO make function
get_target_property(QnToolsBase_INCLUDE_DIR QnToolsBase INCLUDE_DIRECTORIES)
list(TRANSFORM QnToolsBase_INCLUDE_DIR PREPEND -I)
get_target_property(QnToolsCorrection_INCLUDE_DIR QnToolsCorrection INCLUDE_DIRECTORIES)
list(TRANSFORM QnToolsCorrection_INCLUDE_DIR  PREPEND -I)
ROOT_GENERATE_DICTIONARY(G__QnAnalysisBase
        ${HEADERS}
        LINKDEF QnAnalysisBaseLinkDef.h
        OPTIONS
            ${QnToolsBase_INCLUDE_DIR}
            ${QnToolsCorrection_INCLUDE_DIR}
        )
target_link_libraries(QnAnalysisBase PUBLIC AnalysisTreeBase AnalysisTreeInfra ROOT::Core
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




