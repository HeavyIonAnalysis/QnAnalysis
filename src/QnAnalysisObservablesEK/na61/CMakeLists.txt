
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
        v1_mc.hpp
        v1_centrality.cpp
        v1_centrality.hpp
        v1_combine.cpp
        v1_combine.hpp
        plot_v1_pt_y.cpp
        plot_v1_pt_y.hpp
        plot_dv1_dy.cpp
        plot_dv1_dy.hpp plot_v1_centrality.cpp plot_v1_centrality.hpp)
target_link_libraries(na61_main PRIVATE QnAnalysisObservables)
target_precompile_headers(na61_main REUSE_FROM gse)

add_executable(unfold_v1_pt_y_centrality main_unfold_v1_pt_y_centrality.cpp)
target_link_libraries(unfold_v1_pt_y_centrality PRIVATE QnAnalysisObservables)
target_precompile_headers(unfold_v1_pt_y_centrality REUSE_FROM gse)