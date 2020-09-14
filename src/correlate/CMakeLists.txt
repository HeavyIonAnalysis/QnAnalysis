set(SOURCES CorrelationTask.cpp)

set(HEADERS
    CorrelationTask.h
    Utils.h
    Functions.h
        )

add_library(QnAnalysisCorrelate SHARED ${SOURCES} G__QnAnalysisCorrelate.cxx)
link_directories(${PROJECT_LINK_DIRECTORIES})
add_dependencies(QnAnalysisCorrelate ${PROJECT_DEPENDENCIES})
include_directories(${PROJECT_INCLUDE_DIRECTORIES} ${CMAKE_CURRENT_SOURCE_DIR})

ROOT_GENERATE_DICTIONARY(G__QnAnalysisCorrelate ${HEADERS} LINKDEF QnAnalysisCorrelateLinkDef.h)
target_link_libraries(QnAnalysisCorrelate ${ROOT_LIBRARIES})

install(TARGETS QnAnalysisCorrelate EXPORT QnAnalysisTargets
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

set(PCM_FILE_NAME libQnAnalysisCorrelate)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PCM_FILE_NAME}_rdict.pcm"
        DESTINATION
        lib
        OPTIONAL
)