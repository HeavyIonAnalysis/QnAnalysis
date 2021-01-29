
find_package(Boost REQUIRED)

add_executable(QnAnalysisObservables na61.cpp)
target_link_libraries(QnAnalysisObservables PUBLIC
        QnToolsBase ${ROOT_LIBRARIES} ${Boost_LIBRARIES}
        QnAnalysisTools
        )