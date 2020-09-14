#include "Resolution.h"

Result ResolutionTrack(std::vector<Result> args, const std::vector<std::string> &, float y_lo, float y_hi) {
  assert(args.size() == 3);
  // multiplication of the two observables is undefined
  // -> convert to REFERENCE
  auto& nom1 = args[0];
  nom1 = SetReference(nom1);
  nom1 = RebinRapidity(nom1, {y_lo, y_hi}).Projection({"Centrality"});
  auto& nom2 = args[1];
  nom2 = SetReference(nom2);
  nom2 = RebinRapidity(nom2, {y_lo, y_hi}).Projection({"Centrality"});
  const auto& denom = args[2];
  return Qn::Sqrt(nom1 * nom2 / denom);
}

Result Resolution4S(std::vector<Result> args, const std::vector<std::string> &, float y_lo, float y_hi) {
  std::cout << "Hallo!" << std::endl;
  if (args.size() == 3) {
    auto& nom = args[0];
    auto& RT = args[1];
    auto& denom = args[2];
    denom = SetReference(denom);
    denom = RebinRapidity(denom, {y_lo, y_hi}).Projection({"Centrality"});
    nom = nom * (-1); //TODO
    return nom * RT / denom;
  } else if (args.size() == 2) {
    auto& nom = args[0];
    auto& RT = args[1];
    nom = SetReference(nom);
    nom = RebinRapidity(nom, {y_lo, y_hi}).Projection({"Centrality"});
    nom = nom * (-1); //TODO
    return nom / RT;
  }
  assert(0);
}
