set(SOURCES CorrelationTask.cpp)

set(HEADERS
    CorrelationTask.h
    Utils.h
    Functions.h
        )

add_library(FlowCorrelate SHARED ${SOURCES} G__FlowCorrelate.cxx)
link_directories(${PROJECT_LINK_DIRECTORIES})
add_dependencies(FlowCorrelate ${PROJECT_DEPENDENCIES})
include_directories(${PROJECT_INCLUDE_DIRECTORIES} ${CMAKE_CURRENT_SOURCE_DIR})

ROOT_GENERATE_DICTIONARY(G__FlowCorrelate ${HEADERS} LINKDEF FlowCorrelateLinkDef.h)
target_link_libraries(FlowCorrelate ${ROOT_LIBRARIES})

install(TARGETS FlowCorrelate EXPORT FlowTargets
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

set(PCM_FILE_NAME libFlowCorrelate)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PCM_FILE_NAME}_rdict.pcm"
        DESTINATION
        lib
        OPTIONAL
)