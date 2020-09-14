//
// Created by eugene on 9/10/19.
//

#include <iostream>

#include <QnTools/DataContainer.hpp>
#include <QnTools/QVector.hpp>

#include <TTree.h>
#include <TFile.h>
#include <TTreeReaderValue.h>
#include <TTreeReader.h>
#include <TDirectory.h>
#include <TH2D.h>

using namespace std;

TDirectory *mkdirOrGet(TDirectory *tgt, const std::string &dirName) {
  assert(tgt);

  TDirectory *newDir{nullptr};
  tgt->GetObject(dirName.c_str(), newDir);

  if (!newDir) {
    return tgt->mkdir(dirName.c_str());
  }

  return newDir;
}

template<class T>
T *createOrGet(TDirectory *tgt, const std::string &namecycle, T &&proto) {
  assert(tgt);

  T *objPtr{nullptr};
  tgt->GetObject(namecycle.c_str(), objPtr);

  if (objPtr) {
    return objPtr;
  }

  auto *pwd = gDirectory;
  tgt->cd();
  objPtr = new T(proto);
  objPtr->SetName(namecycle.c_str());
  pwd->cd();
  return objPtr;
}

int main(int argc, char **argv) {
  cout << "go!" << endl;

  std::unique_ptr<TFile> inputFile(TFile::Open(argv[1]));

  if (!inputFile) {
    cerr << "No such file or directory" << endl;
    return 1;
  }

  TTree *tree{nullptr};
  inputFile->GetObject("tree", tree);

  if (!tree) {
    cerr << "No such tree" << endl;
    return 1;
  }

  std::unique_ptr<TFile> qaFile(TFile::Open("qvecQA.root", "recreate"));

  TTreeReader treeReader(tree);

  struct QVecQAEntry {
    typedef TTreeReaderValue<Qn::DataContainerQVector> Reader_t;
    std::string name{""};
    std::unique_ptr<Reader_t> qVecContainerReaderPtr_;

    const Qn::DataContainerQVector &getVal() const {
      return qVecContainerReaderPtr_->operator*();
    }

    QVecQAEntry(const string &name,
                Reader_t &&reader)
        : name(name), qVecContainerReaderPtr_(new Reader_t(reader)) {}

  };

  std::vector<QVecQAEntry> qVecQAEntries;

  TObjArray *branchList = tree->GetListOfBranches();

  for (auto *objPtr : *branchList) {
    auto *branchPtr = dynamic_cast<TBranch *>(objPtr);

    std::cout << std::string(branchPtr->GetClassName()) << std::endl;

    if (std::string(branchPtr->GetClassName()) == "Qn::DataContainer<Qn::QVector,Qn::Axis<double> >") {
      qVecQAEntries.emplace_back(
          std::string(branchPtr->GetName()), QVecQAEntry::Reader_t(treeReader, branchPtr->GetName()));

    }
  }

  Long64_t iEvent{0};
  for (auto &qVecQAEntry : qVecQAEntries) {
    cout << qVecQAEntry.name << endl;
    auto *qVecQAEntryTopDir = mkdirOrGet(qaFile.get(), qVecQAEntry.name);

    iEvent = 0;
    treeReader.Restart();
    while (treeReader.Next()) {

      std::cout << "Event # " << iEvent+1 << "\r" << std::flush;

      const auto &qVecContainer = qVecQAEntry.getVal();

      unsigned int iContainerBin = 0;
      for (const auto &qVecPtr : qVecContainer) {
        auto nHarmonics = qVecPtr.GetNoOfHarmonics();
        auto *binDir = mkdirOrGet(qVecQAEntryTopDir, Form("bin_%d", iContainerBin));
        for (decltype(nHarmonics) iHarmonic = 1; iHarmonic < nHarmonics; ++iHarmonic) {
          double qX = qVecPtr.x(iHarmonic);
          double qY = qVecPtr.y(iHarmonic);
          double weight = qVecPtr.sumweights();

          auto *h2QyVsQx =
              createOrGet(binDir, Form("QyVsQx_H%u", iHarmonic), TH2D("test", "", 400, -2, 2, 400, -2, 2));
          h2QyVsQx->Fill(qX, qY);
          auto *h2QyVsQx_weighted =
              createOrGet(binDir, Form("QyVsQxWeighted_H%u", iHarmonic), TH2D("test", "", 400, -2, 2, 400, -2, 2));
          h2QyVsQx_weighted->Fill(qX, qY, weight);
        }

        ++iContainerBin;
      }

      ++iEvent;
    }

    qVecQAEntryTopDir->Write();
    delete qVecQAEntryTopDir;
  }

  if (qaFile) {
    qaFile->Close();
  }

  return 0;
}

