include_directories(${PROJECT_INCLUDE_DIRECTORIES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
link_directories(${PROJECT_LINK_DIRECTORIES})

add_executable(correct Correct.cpp)
add_dependencies(correct ${PROJECT_DEPENDENCIES})
target_link_libraries(correct FlowCorrect FlowBase  QnToolsCorrection QnToolsBase AnalysisTreeBase AnalysisTreeInfra ${ROOT_LIBRARIES})

add_executable(correlate Correlate.cpp)
add_dependencies(correlate ${PROJECT_DEPENDENCIES})
target_link_libraries(correlate FlowCorrelate FlowBase QnToolsBase)

add_executable(analyze Analyze.cpp)
add_dependencies(analyze FlowAna ${PROJECT_DEPENDENCIES})
target_link_libraries(analyze QnToolsBase FlowAna ${ROOT_LIBRARIES})
#
#add_executable(profiles Profiles.cpp)
#add_dependencies(profiles FlowAna ${PROJECT_DEPENDENCIES})
#target_link_libraries(profiles QnToolsBase FlowAna ${ROOT_LIBRARIES})

install(TARGETS correct correlate DESTINATION bin )
#install(TARGETS correct correlate analyze profiles DESTINATION bin )
