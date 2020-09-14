#ifndef FLOW_PROCESSING__FLOW_H_
#define FLOW_PROCESSING__FLOW_H_

#include "Utils.h"

static Result Combine(std::vector<Result> resolutionMHParticles, const std::vector<std::string> &) {
  assert(resolutionMHParticles.size() > 1);
  Result result{resolutionMHParticles.front()};

  int n{1};
  for (auto it = begin(resolutionMHParticles) + 1; it != end(resolutionMHParticles); ++it) {
    result = result.Apply(*it, [](const Qn::Stats &lhs, const Qn::Stats &rhs) { return Qn::Merge(lhs, rhs); });
    ++n;
  }

  return result;
}

static Result FlowV1(std::vector<Result> args, const std::vector<std::string> &) {
  assert(args.size() == 2);
  const auto& uQ = args[0];
  const auto& R = args[1];
  return uQ / R;
}

static Result FlowV1MC(std::vector<Result> args, const std::vector<std::string> &) {
  assert(args.size() == 1);
  const auto& nom1 = args[0];
  return nom1;
}

static Result FlowV2Opt1(std::vector<Result> args, const std::vector<std::string> &argNames) {
  assert(args.size() == 3);

  auto uQQ = args[0];
  // flip sign
  if (argNames[0].find("X2YY") < argNames[0].size()) {
    uQQ = uQQ * (-1);
  }

  auto R1 = args[1];
  auto R2 = args[2];
  return uQQ * 4 / (R1 * R2);
}

static Result FlowV2Opt2(std::vector<Result> args, const std::vector<std::string> &argNames) {
  assert(args.size() == 4);

  auto uQQ1 = args[0];
  auto uQQ2 = args[1];
  // no need to flip sign since Opt2 is irrelevant for
  // X2YY
  auto QQ1 = args[2];
  auto QQ2 = args[3];
  return Qn::Sqrt(uQQ1 * uQQ2 * 2 * 2 / (QQ1 * QQ2)) * 4;
}

static Result FlowV2Opt3(std::vector<Result> args, const std::vector<std::string> &argNames) {
  assert(args.size() == 2);

  auto uQQ = args[0];
  // flip sign
  if (argNames[0].find("X2YY") < argNames[0].size()) {
    uQQ = uQQ * (-1);
  }

  auto QQ = args[1];
  return uQQ * 4 * 2 / QQ;
}

#endif //FLOW_PROCESSING__FLOW_H_
