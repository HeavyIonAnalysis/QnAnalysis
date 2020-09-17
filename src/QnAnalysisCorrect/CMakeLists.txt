set(SOURCES
        QnCorrectionTask.cpp
        ATVarManagerTask.cpp ATVarManagerTask.h)

string(REPLACE ".cpp" ".hpp" HEADERS "${SOURCES}")

add_executable(QnAnalysisCorrect ${SOURCES}
#        G__QnAnalysisCorrect.cxx
        )
include_directories(${PROJECT_INCLUDE_DIRECTORIES} ${QnAnalysis_SOURCE_DIR})

#ROOT_GENERATE_DICTIONARY(G__QnAnalysisCorrect ${HEADERS}
#        LINKDEF QnAnalysisCorrectLinkDef.hpp)
target_include_directories(QnAnalysisCorrect
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${QnAnalysis_SOURCE_DIR})
target_link_libraries(QnAnalysisCorrect PUBLIC QnAnalysisBase QnAnalysisConfig at_task_main)


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




