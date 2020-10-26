//
// Created by mikhail on 10/23/20.
//

#ifndef OBSERVABLESCALCULATOR_SRC_FILE_MANAGER_H_
#define OBSERVABLESCALCULATOR_SRC_FILE_MANAGER_H_

#include <TFile.h>

class FileManager {
public:
  static bool OpenFile( const std::string& name ){
    if( Instance()->file_ )
      Instance()->file_->Close();
    Instance()->file_ = TFile::Open(name.c_str());
    return Instance()->file_ != nullptr;
  }
  template <typename T>
  static T* GetObject(const std::string& name){
    if( !Instance()->file_ )
      throw std::runtime_error("FileManager::GetObject(): file is not specified");
    T* obj;
    Instance()->file_->GetObject(name.c_str(), obj);
    if( !obj )
      throw std::runtime_error("FileManager::GetObject(): there is no object named "+name+" in file");
    return obj;
  }
  static void Cd( const std::string& directory ){
    if( !Instance()->file_ )
      throw std::runtime_error("FileManager::Cd(): file is not specified");
    Instance()->file_->cd(directory.c_str());
  }

private:
  TFile* file_{};
  static FileManager* instance_;
  static FileManager* Instance(){
    if( !instance_ )
      instance_ = new FileManager();
    return instance_;
  }
  FileManager() = default;
  virtual ~FileManager() = default;
};

#endif // OBSERVABLESCALCULATOR_SRC_FILE_MANAGER_H_
