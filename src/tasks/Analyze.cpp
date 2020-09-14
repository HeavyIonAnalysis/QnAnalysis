#include <string>
#include <vector>

#include "analyze/ResourceManager.h"
#include "analyze/Resolution.h"
#include "analyze/FlowCoefficients.h"
#include "analyze/Utils.h"

void DefineResolution3S(ResourceManager& rm);
void DefineResolution4S(ResourceManager& rm);
void DefineResolutionMH(ResourceManager& rm);
void DefineResolutionMC(ResourceManager& rm);

void DefineV1(ResourceManager& rm, const std::vector<std::string>& particles);
void DefineV2(ResourceManager& rm, const std::vector<std::string>& particles);

int main(int argc, char* argv[]){

  if(argc < 3) {
    std::cout << "Error! Please use " << std::endl;
    std::cout << " ./analyze input_name output_name" << std::endl;
    exit(EXIT_FAILURE);
  }

  using std::vector;
  using std::string;

  std::string input_name = argv[1];
  std::string output_name = argv[2];

  std::vector<std::string> particles{
    "proton",
    "pion"
  };

  ResourceManager rm;
  rm.LoadFile(input_name);

  DefineResolution3S(rm);
  DefineResolution4S(rm);
  DefineResolutionMH(rm);
  DefineResolutionMC(rm);

  DefineV1(rm, particles);
//  DefineV2(rm, particles);

  rm.SaveAs(output_name);

  return 0;
}


void DefineResolutionMC(ResourceManager& rm) {
  rm.Define("RES_psd1_MC_X", {"psd1_psi_XX"}, ResolutionMC);
  rm.Define("RES_psd2_MC_X", {"psd1_psi_XX"}, ResolutionMC);
  rm.Define("RES_psd3_MC_X", {"psd3_psi_XX"}, ResolutionMC);
  rm.Define("RES_psd1_MC_Y", {"psd1_psi_YY"}, ResolutionMC);
  rm.Define("RES_psd2_MC_Y", {"psd1_psi_YY"}, ResolutionMC);
  rm.Define("RES_psd3_MC_Y", {"psd3_psi_YY"}, ResolutionMC);
}

void DefineV1(ResourceManager& rm, const std::vector<std::string>& particles) {

  std::vector<std::string> axes{"pT", "rapidity"};
  std::vector<std::string> psd_references{"psd1", "psd2", "psd3"};
  std::vector<std::string> components{"XX", "YY"};
  std::vector<std::string> resolution_methods{"MC"};

  // RES_psd1_3S_X
  const char *R_pattern = "RES_%s_%s_%s";

  // v1_proton_y_psd1_3S_X
  const char *v1_pattern = "v1_%s_%s_%s_%s_%s";

  // proton_psd1_XX
  const char *uQ_pattern = "%s_%s_%s";
  // proton_psd1_XX_pT
  const char *uQ_pattern_int = "%s_%s_%s_%s";

  // mc_proton_psi_XX
  const char *uQ_MC_pattern = "%s_psi_%s";
  // mc_proton_psi_XX_pT
  const char *uQ_MC_pattern_int = "%s_psi_%s_%s";

  auto integrate_pT = Selection("pT", 0, 1);
  auto integrate_rapidity = Selection("rapidity", 1.62179f, 1.62179f+1.0);

  for (const auto &particle : particles) {
    for (const auto &component_2 : components) {
      std::string uQ_name{fmt::sprintf(uQ_MC_pattern, particle, component_2)};
      rm.Define(uQ_name + "_rapidity", {uQ_name}, integrate_pT);
      rm.Define(uQ_name + "_pT", {uQ_name}, integrate_rapidity);
      for (const auto &psd : psd_references) {
        std::string uQ_name{fmt::sprintf(uQ_pattern, particle, psd, component_2)};
        rm.Define(uQ_name + "_rapidity", {uQ_name}, integrate_pT);
        rm.Define(uQ_name + "_pT", {uQ_name}, integrate_rapidity);
      }
    }
  }

  for (const auto &particle : particles) {
    for (const auto &axis : axes) {
      for (const auto &psd_reference : psd_references) {
        for (const auto &component_2 : components) {
          for (const auto &res_method : resolution_methods) {
            auto component_1 = component_2.substr(0, 1);
            std::string uQ_name{fmt::sprintf(uQ_pattern_int, particle, psd_reference, component_2, axis)};
            std::string R_name{fmt::sprintf(R_pattern, psd_reference, res_method, component_1)};
            std::string v1_name{fmt::sprintf(v1_pattern, particle, axis, psd_reference, res_method, component_1)};
            rm.Define(v1_name, {uQ_name, R_name}, FlowV1);
          } // methods
        } // components
      } // psd reference

      for (const auto &res_method : resolution_methods) {
        // combine XY
        for (const auto &psd_reference : psd_references) {
          std::string v1_name{fmt::sprintf("v1_%s_%s_%s_%s_CC", particle, axis, psd_reference, res_method)};
          std::string v1_pattern{fmt::sprintf("v1_%s_%s_%s_%s_(X|Y)", particle, axis, psd_reference, res_method)};
          rm.Define(v1_name, rm.GetMatchingName(v1_pattern), Combine);
        }

//        // combine PSD
//        for (const auto &component_2 : components) {
//          std::string v1_name {fmt::sprintf("v1_%s_%s_%s_%s_CR", particle, axis, res_method, component_2.substr(0, 1))};
//          std::string v1_pattern {fmt::sprintf("v1_%s_%s_psd(1|2|3)_%s_%s", particle, axis, res_method, component_2.substr(0, 1))};
//          rm.Define(v1_name, rm.GetMatchingName(v1_pattern), Combine);
//        }
      }

      // Good combination for 3S method resolution: PSD1 and 3, X component only
      // PSD2 is contaminated by shower leakage
      // y is believed biased due to spectator spot
      {
        const std::string v1_name{fmt::sprintf("v1_%s_%s_3S_CA", particle, axis)};
        const std::string v1_pattern{fmt::sprintf("v1_%s_%s_psd(1|3)_3S_(X)", particle, axis)};
        rm.Define(v1_name, rm.GetMatchingName(v1_pattern), Combine);
      }

      // Good combination for 4S method resolution: PSD1,2,3, X component only
      // y is believed biased due to spectator spot
      {
        const std::string v1_name{fmt::sprintf("v1_%s_%s_4S_CA", particle, axis)};
        const std::string v1_pattern{fmt::sprintf("v1_%s_%s_psd(1|3)_4S_(X)", particle, axis)};
        rm.Define(v1_name, rm.GetMatchingName(v1_pattern), Combine);
      }
    } // axes
  } // particles


  // v1_mc_proton_y_psi_X
  const char *v1_MC_pattern = "v1_%s_%s_psi_%s";

  for (const auto &particle : particles) {
    for (const auto &component_2 : components) {
      for (const auto &axis : axes) {
        auto component_1 = component_2.substr(0, 1);
        std::string uQ_name{fmt::sprintf(uQ_MC_pattern_int, particle, component_2, axis)};
        std::string v1_name{fmt::sprintf(v1_MC_pattern, particle, axis, component_1)};
        rm.Define(v1_name, {uQ_name}, FlowV1MC);
      } // components
    } // axes
  } // particles
}

void DefineV2(ResourceManager& rm, const std::vector<std::string>& particles){

  std::vector<std::string> axes{"pT", "y"};
  std::vector<std::string> components{"X", "Y"};
  std::vector<std::string> refPsd{"psd1", "psd2", "psd3"};
  std::vector<std::string> refComponents{"XX", "YY", "XY", "YX"};

  for (const auto &particle : particles) {
    for (const auto &axis : axes) {
      for (const auto &refPSD1 : refPsd) {
        for (const auto &refPSD2 : refPsd) {
          if (refPSD1 == refPSD2) continue;

          for (const auto &component : components) {

            for (const auto &ref_component : refComponents) {
              /*********** opt 1 ***********/
              // this option is relevant for all combinations
              // let this to be default
              if (ref_component == "XY" || ref_component == "YX") {
                auto v2_name{fmt::sprintf("v2_%s_%s_%s_%s_%s2%s", particle, axis, refPSD1, refPSD2, component, ref_component)};
                auto uQQ_name{fmt::sprintf("%s_%s_%s_%s_%s2%s", particle, axis, refPSD1, refPSD2, component, ref_component)};

                auto R1_name{fmt::sprintf("RES_%s_3S_%s", refPSD1, std::string(ref_component.begin(), ref_component.begin() + 1))};
                auto R2_name{fmt::sprintf("RES_%s_3S_%s", refPSD2, std::string(ref_component.begin() + 1, ref_component.begin() + 2)
                )};

                rm.Define(v2_name, {uQQ_name, R1_name, R2_name}, FlowV2Opt1);
              }
              /*********** opt 2 ***********/
              if (ref_component == "XY" || ref_component == "YX") {
                auto v2_name(fmt::sprintf("v2_%s_%s_%s_%s_%s2%s", particle, axis, refPSD1, refPSD2, component, ref_component));

                auto uQQ1_name{fmt::sprintf("%s_%s_%s_%s_%s2%s", particle, axis, refPSD1, refPSD2, component, ref_component)};
                auto uQQ2_name{fmt::sprintf("%s_%s_%s_%s_%s2%s",
                                            particle,
                                            axis,
                                            refPSD1,
                                            refPSD2,
                                            component,
                                            std::string(ref_component.rbegin(), ref_component.rend())
                )};
                auto QQ1_name{fmt::sprintf("%s_%s_XX", refPSD1, refPSD2)};
                auto QQ2_name{fmt::sprintf("%s_%s_YY", refPSD1, refPSD2)};
//                  rm.Define(v2_name, {uQQ1_name, uQQ2_name, QQ1_name, QQ2_name}, FlowV2Opt2);
              }
              /*********** opt 3 ***********/
              if (ref_component == "XX" || ref_component == "YY") {
                auto v2_name(fmt::sprintf("v2_%s_%s_%s_%s_%s2%s", particle, axis, refPSD1, refPSD2, component, ref_component));
                auto uQQ_name{fmt::sprintf("%s_%s_%s_%s_%s2%s", particle, axis, refPSD1, refPSD2, component, ref_component)};
                auto QQ_name{fmt::sprintf("%s_%s_%s", refPSD1, refPSD2, ref_component)};
                rm.Define(v2_name, {uQQ_name, QQ_name}, FlowV2Opt3);

              }

            } // ref components

          } // component


          // combine non-zero components
          {
            auto v2_pattern{fmt::sprintf("v2_%s_%s_%s_%s_(X2XX|X2YY|Y2XY|Y2YX)", particle, axis, refPSD1, refPSD2)};
            auto v2_name{fmt::sprintf("v2_%s_%s_%s_%s_CC", particle, axis, refPSD1, refPSD2)};
            rm.Define(v2_name, rm.GetMatchingName(v2_pattern), Combine);
          }
        } // ref PSD2
      } // ref PSD1

      // combine reference PSD
      for (const auto &component : {"X2XX", "X2YY", "Y2YX", "Y2XY"}) {
        auto v2_pattern{fmt::sprintf("v2_%s_%s_psd(1|2|3)_psd(1|2|3)_%s", particle, axis, component)};
        auto v2_name{fmt::sprintf("v2_%s_%s_%s_CR", particle, axis, component)};
        rm.Define(v2_name, rm.GetMatchingName(v2_pattern), Combine);
      }

      // Good combination: all PSDs and Y2XY & Y2YX only
      {
        auto v2_pattern{fmt::sprintf("v2_%s_%s_psd(1|2|3)_psd(1|2|3)_(Y2XY|Y2YX)", particle, axis)};
        auto v2_name{fmt::sprintf("v2_%s_%s_CA", particle, axis)};
        rm.Define(v2_name, rm.GetMatchingName(v2_pattern), Combine);
      }

    } // axes
  } // particles
}


void DefineResolution4S(ResourceManager& rm) {
  using namespace std::placeholders;
  const float y_cuts[] = {0.0+1.62179, 0.8+1.62179};

  auto resolutionTrack_fct = std::bind(&ResolutionTrack, _1, _2, y_cuts[0], y_cuts[1]);
  auto resolution4S_fct = std::bind(&Resolution4S, _1, _2, y_cuts[0], y_cuts[1]);

  rm.Define("RES_psd1_psd3_T_X", {"pion_neg_psd1_XX", "pion_neg_psd3_XX", "psd3_psd1_XX"}, resolutionTrack_fct);
  rm.Define("RES_psd1_psd3_T_Y", {"pion_neg_psd1_YY", "pion_neg_psd3_YY", "psd3_psd1_YY"}, resolutionTrack_fct);

  rm.Define("RES_psd1_4S_X", {"psd3_psd1_XX", "RES_psd1_psd3_T_X", "pion_neg_psd3_XX"}, resolution4S_fct);
  rm.Define("RES_psd2_4S_X", {"pion_neg_psd2_XX", "RES_psd1_psd3_T_X"}, resolution4S_fct);
  rm.Define("RES_psd3_4S_X", {"psd3_psd1_XX", "RES_psd1_psd3_T_X", "pion_neg_psd1_XX"}, resolution4S_fct);

  rm.Define("RES_psd1_4S_Y", {"psd3_psd1_YY", "RES_psd1_psd3_T_Y", "pion_neg_psd3_YY"}, resolution4S_fct);
  rm.Define("RES_psd2_4S_Y", {"pion_neg_psd2_YY", "RES_psd1_psd3_T_Y"}, resolution4S_fct);
  rm.Define("RES_psd3_4S_Y", {"psd3_psd1_YY", "RES_psd1_psd3_T_Y", "pion_neg_psd1_YY"}, resolution4S_fct);
}

void DefineResolutionMH(ResourceManager& rm) {
  /** Mixed-harmonics **/
  ResolutionMH resolutionMH{-0.6+1.62179, 0+1.62179};
  std::vector<std::string> ref_particles{"proton", "pion", "pion_neg"};

  for(const auto& particle : ref_particles){

    rm.Define("RES_psd1_MH_X1_"+particle ,
              {particle +"_psd1_psd2_X2XX", "psd3_psd1_XX", particle +"_psd2_psd3_X2XX"},
              resolutionMH);
    rm.Define("RES_psd1_MH_X2_"+particle ,
              {particle +"_psd1_psd2_Y2XY", "psd3_psd1_XX", particle +"_psd2_psd3_Y2YX"},
              resolutionMH);
    rm.Define("RES_psd1_MH_X_"+particle , {"RES_psd1_MH_X1_"+particle , "RES_psd1_MH_X2_"+particle ,}, Combine);

    rm.Define("RES_psd1_MH_Y1_"+particle ,
              {particle +"_psd1_psd2_X2YY", "psd3_psd1_YY", particle +"_psd2_psd3_Y2XY"},
              resolutionMH);
    rm.Define("RES_psd1_MH_Y2_"+particle ,
              {particle +"_psd1_psd2_Y2YX", "psd3_psd1_YY", particle +"_psd2_psd3_Y2XY"},
              resolutionMH);
    rm.Define("RES_psd1_MH_Y_"+particle , {"RES_psd1_MH_Y1_"+particle , "RES_psd1_MH_Y2_"+particle ,}, Combine);

    rm.Define("RES_psd3_MH_X1_"+particle ,
              {particle +"_psd2_psd3_X2XX", "psd3_psd1_XX", particle +"_psd1_psd2_X2XX"},
              resolutionMH);
    rm.Define("RES_psd3_MH_X2_"+particle ,
              {particle +"_psd2_psd3_Y2YX", "psd3_psd1_XX", particle +"_psd1_psd2_Y2XY"},
              resolutionMH);
    rm.Define("RES_psd3_MH_X_"+particle , {"RES_psd3_MH_X1_"+particle , "RES_psd3_MH_X2_"+particle ,}, Combine);

    rm.Define("RES_psd3_MH_Y1_"+particle ,
              {particle +"_psd2_psd3_Y2YX", "psd3_psd1_YY", particle +"_psd1_psd2_Y2YX"},
              resolutionMH);
    rm.Define("RES_psd3_MH_Y2_"+particle ,
              {particle +"_psd2_psd3_X2YY", "psd3_psd1_YY", particle +"_psd1_psd2_X2YY"},
              resolutionMH);
    rm.Define("RES_psd3_MH_Y_"+particle , {"RES_psd3_MH_Y1_"+particle , "RES_psd3_MH_Y2_"+particle ,}, Combine);

  }
}

void DefineResolution3S(ResourceManager& rm) {
  rm.Define("RES_psd1_3S_X", {"psd1_psd2_XX", "psd3_psd1_XX", "psd2_psd3_XX"}, Resolution3Sub);
  rm.Define("RES_psd2_3S_X", {"psd1_psd2_XX", "psd2_psd3_XX", "psd3_psd1_XX"}, Resolution3Sub);
  rm.Define("RES_psd3_3S_X", {"psd3_psd1_XX", "psd2_psd3_XX", "psd1_psd2_XX"}, Resolution3Sub);

  rm.Define("RES_psd1_3S_Y", {"psd1_psd2_YY", "psd3_psd1_YY", "psd2_psd3_YY"}, Resolution3Sub);
  rm.Define("RES_psd2_3S_Y", {"psd1_psd2_YY", "psd2_psd3_YY", "psd3_psd1_YY"}, Resolution3Sub);
  rm.Define("RES_psd3_3S_Y", {"psd3_psd1_YY", "psd2_psd3_YY", "psd1_psd2_YY"}, Resolution3Sub);
}
