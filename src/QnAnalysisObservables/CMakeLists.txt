set(SOURCES
        FileManager.cpp
        Method.cpp
        MethodOf3SE.cpp
        V1Observables.cpp
        )

add_executable(flow Calculate.cpp ${SOURCES})
target_link_libraries(flow QnTools::Base ${ROOT_LIBRARIES})