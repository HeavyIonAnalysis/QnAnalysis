//
// Created by eugene on 04/01/2021.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLES_TOOLS_HPP_
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLES_TOOLS_HPP_

#include <DataContainer.hpp>
#include <DataContainerHelper.hpp>
#include <filesystem>
#include <TClass.h>
#include <TFile.h>
#include <TKey.h>

namespace Tools {

template<typename T>
void ExportToROOT(const char *filename, const char *mode = "RECREATE") {
  TFile f(filename, mode);
  ResourceManager::Instance().ForEach([&f](const std::string &name, T &value) {
                                        using std::filesystem::path;
                                        path p(name);

                                        auto dname = p.parent_path().relative_path();
                                        auto bname = p.filename();
                                        auto save_dir = f.GetDirectory(dname.c_str(), true);
                                        if (!save_dir) {
                                          std::cout << "mkdir " << f.GetName() << ":" << dname.c_str() << std::endl;
                                          f.mkdir(dname.c_str(), "");
                                          save_dir = f.GetDirectory(dname.c_str(), true);
                                        }
                                        save_dir->WriteTObject(&value, bname.c_str());
                                      },
                                      ResourceManager::AlwaysTrue(), false);
}
}

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLES_TOOLS_HPP_
