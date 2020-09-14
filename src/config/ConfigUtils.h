//
// Created by eugene on 12/08/2020.
//

#ifndef FLOW_SRC_CONFIG_CONFIGUTILS_H
#define FLOW_SRC_CONFIG_CONFIGUTILS_H

#include <sstream>
#include <string>
#include <vector>

namespace Flow::Config::Utils {

inline std::vector<std::string> TokenizeString(const std::string& s, char delim) {
  using namespace std;
  vector<string> result;
  stringstream ss(s);
  string item;

  while (getline(ss, item, delim)) {
    result.push_back(item);
  }

  return result;
}

template<typename T>
inline std::vector<T> EmptyVector() { return std::vector<T>(); }

}// namespace Flow::Config::Utils

#endif//FLOW_SRC_CONFIG_CONFIGUTILS_H
