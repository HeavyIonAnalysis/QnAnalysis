set(SOURCES
        CorrectTaskManager.cpp
        CorrectionTask.cpp
        )

string(REPLACE ".cpp" ".h" HEADERS "${SOURCES}")

add_library(FlowCorrect SHARED ${SOURCES} G__FlowCorrect.cxx)
link_directories(${PROJECT_LINK_DIRECTORIES})
add_dependencies(FlowCorrect ${PROJECT_DEPENDENCIES} FlowBase)
include_directories(${PROJECT_INCLUDE_DIRECTORIES} ${CMAKE_CURRENT_SOURCE_DIR})

ROOT_GENERATE_DICTIONARY(G__FlowCorrect ${HEADERS} LINKDEF FlowCorrectLinkDef.h)
target_link_libraries(FlowCorrect FlowBase FlowConfig AnalysisTreeBase AnalysisTreeInfra ${ROOT_LIBRARIES} )

install(TARGETS FlowCorrect EXPORT FlowCorrectTargets
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

set(PCM_FILE_NAME libFlowCorrect)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PCM_FILE_NAME}_rdict.pcm"
        DESTINATION
        lib
        OPTIONAL
)




