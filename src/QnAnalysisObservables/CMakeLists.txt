find_package(Boost REQUIRED COMPONENTS program_options)

set(SOURCES
        FileManager.cpp
        Method.cpp
        MethodOf3SE.cpp
        V1Observables.cpp
        MethodOfRS.cpp
        ConvertConfigs.cpp)

add_executable(BuildObservables Calculate.cpp ${SOURCES})
target_link_libraries(BuildObservables QnTools::Base Boost::program_options ${ROOT_LIBRARIES} yaml-cpp)