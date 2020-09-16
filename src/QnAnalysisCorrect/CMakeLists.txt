set(SOURCES
        TaskManager.cpp
        Task.cpp
        )

string(REPLACE ".cpp" ".hpp" HEADERS "${SOURCES}")

add_library(QnAnalysisCorrect STATIC ${SOURCES}
#        G__QnAnalysisCorrect.cxx
        )
include_directories(${PROJECT_INCLUDE_DIRECTORIES} ${QnAnalysis_SOURCE_DIR})
message("${PROJECT_INCLUDE_DIRECTORIES}")

#ROOT_GENERATE_DICTIONARY(G__QnAnalysisCorrect ${HEADERS}
#        LINKDEF QnAnalysisCorrectLinkDef.hpp)
target_link_libraries(QnAnalysisCorrect PUBLIC QnAnalysisBase QnAnalysisConfig at_task)

install(TARGETS QnAnalysisCorrect EXPORT QnAnalysisCorrectTargets
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

set(PCM_FILE_NAME libQnAnalysisCorrect)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PCM_FILE_NAME}_rdict.pcm"
        DESTINATION
        lib
        OPTIONAL
)




