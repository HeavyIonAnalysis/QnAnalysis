find_package(Boost REQUIRED COMPONENTS program_options)

set(SOURCES
        FileManager.cpp
        Method.cpp
        MethodOf3SE.cpp
        V1Observables.cpp
        MethodOfRS.cpp
        ConvertConfigs.cpp)

add_executable(flow Calculate.cpp ${SOURCES})
target_link_libraries(flow QnTools::Base Boost::program_options ${ROOT_LIBRARIES} yaml-cpp)