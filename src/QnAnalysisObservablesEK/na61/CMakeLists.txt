
add_executable(na61_main
        na61.cpp
        remap_axis.cpp
        remap_axis.hpp
        resolution_3sub.cpp
        resolution_3sub.hpp
        resolution_mc.cpp
        resolution_mc.hpp
        resolution_4sub.cpp
        resolution_4sub.hpp
        v1_reco.cpp
        v1_reco.hpp
        v1_mc.cpp
        v1_mc.hpp v1_centrality.cpp v1_centrality.hpp v1_combine.cpp v1_combine.hpp)
target_link_libraries(na61_main PRIVATE QnAnalysisObservables)