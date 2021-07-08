//
// Created by eugene on 27/10/2020.
//

#include <TFile.h>

#include "TStringMeta.hpp"
#include <gtest/gtest.h>


namespace {


TEST(TStringMeta, Same_Meta) {

  TStringMeta s1(std::string("test"));
  TStringMeta s2(std::string("test"));

  TList l;
  l.SetOwner(kFALSE);
  l.Add(&s2);

  s1.Merge(&l);

}





}

