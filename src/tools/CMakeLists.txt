include_directories(${PROJECT_INCLUDE_DIRECTORIES} ${CMAKE_CURRENT_SOURCE_DIR})
link_directories(${PROJECT_LINK_DIRECTORIES})

add_executable(qvector_qa QVectorQA.cxx)
add_dependencies(qvector_qa ${PROJECT_DEPENDENCIES})
target_link_libraries(qvector_qa QnToolsBase ${ROOT_LIBRARIES} )

install(TARGETS qvector_qa DESTINATION bin )
