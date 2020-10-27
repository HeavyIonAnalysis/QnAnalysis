//
// Created by eugene on 27/10/2020.
//

#include <iostream>
#include "TStringMeta.hpp"
#include <TCollection.h>

ClassImp(TStringMeta)

Bool_t TStringMeta::IsEqual(const TObject *other) const {
  if (this == other) return kTRUE;
  if (TStringMeta::Class() != other->IsA()) return false;
  return TObjString::IsEqual((TObjString*) other);
}

Long64_t TStringMeta::Merge(TCollection *coll) {

  std::cout << "Merge!" << std::endl;
  TIter iter(coll);
  while (TObject *obj = iter()) {
    if (IsEqual(obj)) { /* do nothing */ }
    else {
      return -1;
    }
  }
  return 1l;
}
