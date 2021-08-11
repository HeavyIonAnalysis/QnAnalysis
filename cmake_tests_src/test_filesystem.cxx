//
// Created by eugene on 11/08/2021.
//

#include <filesystem>
#include <iostream>

int main() {
  std::filesystem::path root_path{"/"};
  auto current_path = std::filesystem::current_path();
  std::cout << root_path << std::endl;
  std::cout << current_path << std::endl;
  return 0;
}