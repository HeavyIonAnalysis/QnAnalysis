//
// Created by eugene on 27/10/2020.
//

#ifndef QNANALYSIS_SRC_QNANALYSISCORRELATE_TSTRINGMETA_HPP
#define QNANALYSIS_SRC_QNANALYSISCORRELATE_TSTRINGMETA_HPP

#include <TObjString.h>

class TStringMeta : public TObjString {

public:
  TStringMeta() = default;
  explicit TStringMeta(const std::string& str) : TObjString(str.c_str()) {}
  explicit TStringMeta(const TObjString& ostr) : TObjString(ostr) {}


  Bool_t IsEqual(const TObject *other) const override;
  Long64_t Merge(TCollection *coll);

  ClassDefOverride(TStringMeta, 1);
};

#endif //QNANALYSIS_SRC_QNANALYSISCORRELATE_TSTRINGMETA_HPP
