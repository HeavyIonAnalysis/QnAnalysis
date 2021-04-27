#include "AnalysisTree/Cuts.hpp"
#include "CutsRegistry.hpp"

int CutsData() {
    using namespace AnalysisTree;

    SimpleCut vtx_x(Variable("RecEventHeader","vtx_x"), -0.740, 0.131);
    SimpleCut vtx_y(Variable("RecEventHeader","vtx_y"), -0.664, 0.022);
    SimpleCut vtx_z(Variable("RecEventHeader","vtx_z"), -594., -590.);

    SimpleCut vtx_x_pbpb30_3sigma(Variable("RecEventHeader","vtx_x"), -0.168, 0.684);
    SimpleCut vtx_y_pbpb30_3sigma(Variable("RecEventHeader","vtx_y"), -0.467, 0.073);
    SimpleCut vtx_z_pbpb30_preliminary(Variable("RecEventHeader","vtx_z"), -593., -591.);
    SimpleCut fitted_vtx(Variable("RecEventHeader","fitted_vtx"), 0.9, 1.1);
    SimpleCut vtx_z_magic({"RecEventHeader/vtx_z"}, [](std::vector<double>& args) -> bool { 
            double vtx_z = args[0];
            double vtx_z0 = -591.9;
            return !(TMath::Abs(vtx_z - vtx_z0) < 1e-3);
            });
    SimpleCut e_psd(Variable("RecEventHeader","Epsd"), 0.0000001, 1e9);
    /* Clean up from event overlap remnants */
    SimpleCut e_psd_event_overlap({"Centrality/Epsd", "Centrality/Mgood"},
                                  [] (const std::vector<double>& args) {
      double e_psd = args[0];
      double m_good = args[1];
                                    return e_psd < 3300 - m_good*(3300./200.);
    });
    SimpleCut mgood_gt_0(Variable("Centrality","Mgood"), 1, 1e6);

    SimpleCut wfa_s1(Variable("RecEventHeader","wfa_s1"), 4000.f, 1e9);
    SimpleCut wfa_t4(Variable("RecEventHeader","wfa_t4"), 4000.f, 1e9);

    SimpleCut t4(Variable("RecEventHeader","t4"), 0.9, 1.1);
    SimpleCut t2(Variable("RecEventHeader","t2"), 0.9, 1.1);
    SimpleCut t2ort4({"RecEventHeader/t2", "RecEventHeader/t4"}, [](std::vector<double>& t2t4) { 
            return TMath::Abs(t2t4[0] - 1) < 0.1 || TMath::Abs(t2t4[1] - 1) < 0.1;
            });


    SimpleCut s1(Variable("RecEventHeader","s1"), 115, 175);
    SimpleCut s2(Variable("RecEventHeader","s2"), 150, 500);
    SimpleCut s1_s2({"RecEventHeader/s1", "RecEventHeader/s2"}, [](std::vector<double>& s) { return s[0]*1.258 + s[1] < 390; } );


    RegisterCuts("na61/pbpb/13agev/16_025/minbias/standard/event", Cuts("RecEventHeader", {
                vtx_x,
                vtx_y,
                vtx_z,
                fitted_vtx,
                vtx_z_magic,
                mgood_gt_0,
                e_psd,
                e_psd_event_overlap,
                wfa_s1,
                wfa_t4,
                t2ort4,
                s1, s2, s1_s2
                }));
    RegisterCuts("na61/pbpb/13agev/16_025/t2t4/preliminary/event", Cuts("RecEventHeader", {
                vtx_x,
                vtx_y,
                vtx_z,
                fitted_vtx,
                vtx_z_magic,
                e_psd,
                e_psd_event_overlap,
                mgood_gt_0,
                wfa_s1,
//                wfa_t4,
                t2ort4,
                s1, s2, s1_s2
                }));

     RegisterCuts("na61/pbpb/30agev/16_025/t4/preliminary/event", Cuts("RecEventHeader", {
                vtx_x_pbpb30_3sigma,
                vtx_y_pbpb30_3sigma,
                vtx_z_pbpb30_preliminary,
                fitted_vtx,
                vtx_z_magic,
                e_psd,
//                e_psd_event_overlap,
                mgood_gt_0,
                wfa_s1,
//                wfa_t4,
                t4,
//                s1, s2, s1_s2
                }));
    return 0;
}
