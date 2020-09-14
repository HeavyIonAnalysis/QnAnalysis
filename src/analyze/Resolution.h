#ifndef FLOW_PROCESSING__RESOLUTION_H_
#define FLOW_PROCESSING__RESOLUTION_H_

#include "Utils.h"

[[maybe_unused]] static Result Resolution3Sub(std::vector<Result> args, const std::vector<std::string> &) {
  assert(args.size() == 3);

  const auto& nom1 = args[0];
  const auto& nom2 = args[1];
  const auto& denom = args[2];
  return Qn::Sqrt(nom1 * nom2 / denom);
}

[[maybe_unused]] static Result ResolutionMC(std::vector<Result> args, const std::vector<std::string> &) {
  assert(args.size() == 1);
  const auto& nom1= args[0];
  return nom1;
}

Result ResolutionTrack(std::vector<Result> args, const std::vector<std::string> &, float y_lo, float y_hi);

Result Resolution4S(std::vector<Result> args, const std::vector<std::string> &, float y_lo, float y_hi);

class ResolutionMH {
 public:
  ResolutionMH () = delete;
  ResolutionMH (float y_min, float y_max) :
    y_min_(y_min), y_max_(y_max) {};

  Result operator()(std::vector<Result> args, const std::vector<std::string> &argNames) const {
    assert(args.size() == 3);

    auto& nom1 = args[0];
    auto& nom2 = args[1];
    auto& denom = args[2];

    // flip sign
    if (argNames[0].find("Q2x_Q1y_Q1y") < argNames[0].size()) {
      nom1 = nom1 * (-1);
    }

    // flip sign
    if (argNames[2].find("Q2x_Q1y_Q1y") < argNames[2].size()) {
      denom = denom * (-1);
    }

    nom1 = RebinRapidity(nom1, {y_min_, y_max_}).Projection({"Centrality"});
    denom = RebinRapidity(denom, {y_min_, y_max_}).Projection({"Centrality"});

    return Qn::Sqrt(nom1 * nom2 / denom);
  }
 private:
  float y_min_{0};
  float y_max_{0};
};

#endif //FLOW_PROCESSING__RESOLUTION_H_
