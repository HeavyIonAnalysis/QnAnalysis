set(SOURCES
        Utils.cpp
        Resolution.cpp
        FlowCoefficients.cpp
        ResourceManager.cpp
        )

string(REPLACE ".cpp" ".h" HEADERS "${SOURCES}")

add_library(FlowAna SHARED ${SOURCES} G__FlowAna.cxx)
link_directories(${PROJECT_LINK_DIRECTORIES})
add_dependencies(FlowAna ${PROJECT_DEPENDENCIES})
include_directories(${PROJECT_INCLUDE_DIRECTORIES} ${CMAKE_CURRENT_SOURCE_DIR})

ROOT_GENERATE_DICTIONARY(G__FlowAna ${HEADERS} LINKDEF FlowAnaLinkDef.h)
target_link_libraries(FlowAna ${ROOT_LIBRARIES} )

install(TARGETS FlowAna EXPORT FlowAnaTargets
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

set(PCM_FILE_NAME libFlowAna)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PCM_FILE_NAME}_rdict.pcm"
        DESTINATION
        lib
        OPTIONAL
)

