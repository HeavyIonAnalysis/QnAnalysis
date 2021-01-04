//
// Created by eugene on 04/01/2021.
//

#ifndef QNANALYSIS_SRC_QNANALYSISOBSERVABLES_TOOLS_HPP_
#define QNANALYSIS_SRC_QNANALYSISOBSERVABLES_TOOLS_HPP_

#include <DataContainer.hpp>
#include <DataContainerHelper.hpp>
#include <filesystem>
#include <utility>
#include <TClass.h>
#include <TFile.h>
#include <TKey.h>

namespace Tools {

template <typename T>
struct ToRoot {
  explicit ToRoot(std::string filename, std::string mode = "RECREATE") : filename(std::move(filename)), mode(std::move(mode)) {}

  ToRoot(const ToRoot<T>& other) {
    filename = other.filename;
    mode = other.mode;
  }
  ToRoot(ToRoot<T>&& other)  noexcept = default;

  void operator () (const std::string& fpathstr, T& obj) {
    if (!file) {
      file.reset(TFile::Open(filename.c_str(), mode.c_str()));
    }

    using std::filesystem::path;
    path p(fpathstr);

    auto dname = p.parent_path().relative_path();
    auto bname = p.filename();
    auto save_dir = file->GetDirectory(dname.c_str(), true);

    if (!save_dir) {
      std::cout << "mkdir " << file->GetName() << ":" << dname.c_str() << std::endl;
      file->mkdir(dname.c_str(), "");
      save_dir = file->GetDirectory(dname.c_str(), true);
    }
    save_dir->WriteTObject(&obj, bname.c_str());
  }

  std::string filename;
  std::string mode{"RECREATE"};
  std::unique_ptr<TFile> file{};
};

}

#endif //QNANALYSIS_SRC_QNANALYSISOBSERVABLES_TOOLS_HPP_
