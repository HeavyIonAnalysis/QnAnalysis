#include "ResourceManager.h"

void ResourceManager::LoadFile(const std::string& fileName) {
  TFile ff(fileName.c_str(), "read");

  if (!ff.IsOpen()) {
    Error(__func__, "Unable to open %s", fileName.c_str());
    return;
  }

  auto keys = ff.GetListOfKeys();

  for (auto o : *keys) {
    auto key = dynamic_cast<TKey*>(o);

    std::string objName{key->GetName()};

    std::shared_ptr<Result> resPtr;

    if (GetResource(&ff, objName, resPtr)) {
      auto insertResult = resourceMap.insert({objName, resPtr});
      if (!insertResult.second) {
        Warning(__func__, "Object of name '%s' already exists", objName.c_str());
      } else {
        Info(__func__, "Resource '%s' loaded", objName.c_str());
      }
    } else {
      Error(__func__, "Object of name '%s' not found", objName.c_str());
    }
  }
}

void ResourceManager::SaveAs(const std::string& filename) {
  TFile ff(filename.c_str(), "recreate");

  ff.cd();
  for (auto& entry : resourceMap) {
    entry.second->Write(entry.first.c_str());
  }

  ff.Close();
  Info(__func__, "Saved into '%s'", filename.c_str());
}

void ResourceManager::ForMatchingExec(const std::string& pattern,
                                      const std::function<void(const std::string&, Result)>& fct) const {
  int i_m = 0;
  std::regex re(pattern);
  for (const auto& entry : resourceMap) {
    auto name = entry.first;
    if (std::regex_match(name, re)) {
      fct(name, *entry.second);
      i_m++;
    }
  }
}

std::vector<std::string> ResourceManager::GetMatchingName(const std::string& pattern) {
  std::vector<std::string> result{};

  std::regex re(pattern);
  for (const auto& entry : resourceMap) {
    auto name = entry.first;
    if (std::regex_match(name, re)) {
      result.push_back(name);
    }
  }

  return result;
}
