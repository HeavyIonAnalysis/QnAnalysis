set(SOURCES
        Axis.cpp
        Cut.cpp
        QVector.cpp
        AnalysisSetup.cpp
        Variable.cpp
        Histogram.cpp)

string(REPLACE ".cpp" ".h" HEADERS "${SOURCES}")

add_library(FlowBase SHARED ${SOURCES} G__FlowBase.cxx)
target_include_directories(FlowBase PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>/src
        )
message(${PROJECT_LINK_DIRECTORIES})
link_directories(${PROJECT_LINK_DIRECTORIES})
add_dependencies(FlowBase ${PROJECT_DEPENDENCIES})
include_directories(${PROJECT_INCLUDE_DIRECTORIES} ${CMAKE_CURRENT_SOURCE_DIR})

ROOT_GENERATE_DICTIONARY(G__FlowBase ${HEADERS} LINKDEF FlowBaseLinkDef.h)
target_link_libraries(FlowBase PUBLIC AnalysisTreeBase AnalysisTreeInfra ROOT::Core
        QnToolsBase
        QnToolsCorrection
        )

install(TARGETS FlowBase EXPORT FlowTargets
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

set(PCM_FILE_NAME libFlowBase)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PCM_FILE_NAME}_rdict.pcm"
        DESTINATION
        lib
        OPTIONAL
)




