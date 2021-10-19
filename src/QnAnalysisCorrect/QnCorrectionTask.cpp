#include "QnCorrectionTask.hpp"

#include <iostream>
#include <memory>

#include <AnalysisTree/DataHeader.hpp>

#include <AnalysisTree/AnalysisTreeVersion.hpp>
#if ANALYSISTREE_VERSION_MAJOR == 1
#include <AnalysisTree/TreeReader.hpp>
#elif ANALYSISTREE_VERSION_MAJOR == 2
#include <AnalysisTree/infra-1.0/TreeReader.hpp>
#endif

#include <QnAnalysisBase/QVector.hpp>
#include <QnAnalysisBase/AnalysisSetup.hpp>
#include <QnAnalysisConfig/Config.hpp>

#include <QnAnalysisCorrect/ATVarManagerTask.hpp>

namespace Qn::Analysis::Correction {

TASK_IMPL(QnCorrectionTask)

using std::string;
using std::vector;

void QnCorrectionTask::PreInit() {
  auto at_vm_task = ATVarManagerTask::Instance();
  if (!at_vm_task->IsEnabled()) {
    throw std::runtime_error("Keep ATVarManagerTask enabled");
  }

  this->analysis_setup_ =
      new Base::AnalysisSetup(Qn::Analysis::Config::ReadSetupFromFile(yaml_config_file_, yaml_config_node_));

  // Variables used by tracking Q-vectors
  for (auto &q_tra : this->GetConfig()->track_qvectors_) {
    const auto &vars = q_tra->GetListOfVariables();
    q_tra->SetVarEntryId(at_vm_task->AddEntry(AnalysisTree::VarManagerEntry(vars)).first);
  }
  // Variables used by channelized Q-vectors
  for (auto &q_ch : this->GetConfig()->channel_qvectors_) {
    /* phi variable is 'virtual' and taken from the DataHeader */
    q_ch->SetVarEntryId(at_vm_task->AddEntry(AnalysisTree::VarManagerEntry({q_ch->GetWeightVar()})).first);
  }
  // Psi variable
  for (auto &q_psi : this->GetConfig()->psi_qvectors_) {
    q_psi->SetVarEntryId(at_vm_task->AddEntry(AnalysisTree::VarManagerEntry({q_psi->GetPhiVar()})).first);
  }
  // Event Variables
  if (!this->GetConfig()->EventVars().empty()) {
    at_vm_task->AddEntry(AnalysisTree::VarManagerEntry(this->GetConfig()->GetEventVars()));
  }

  at_vm_task->FillBranchNames();
//  at_vm_task->SetCutsMap(cuts_map_); FIXME

  var_manager_ = at_vm_task;
}

void QnCorrectionTask::UserInit(std::map<std::string, void *> &) {
  out_file_ = std::shared_ptr<TFile>(TFile::Open("correction_out.root", "recreate"));
  if (!(out_file_ && out_file_->IsOpen())) {
    throw std::runtime_error("Unable to open output file for writing");
  }
  out_file_->cd();
  out_tree_ = new TTree("tree", "tree");
  manager_.SetCalibrationInputFileName(in_calibration_file_name_);
  manager_.SetFillOutputTree(true);
  manager_.SetFillCalibrationQA(true);
  manager_.SetFillValidationQA(true);
  manager_.ConnectOutputTree(out_tree_);

  InitVariables();

  for (const auto &axis : analysis_setup_->GetCorrectionAxes()) {
    manager_.AddCorrectionAxis(axis);
  }

  for (const auto &qvec_ptr : analysis_setup_->q_vectors) {
    if (qvec_ptr->GetType() == Base::EQVectorType::TRACK) {
      auto track_qv = std::dynamic_pointer_cast<Base::QVectorTrack>(qvec_ptr);
      const string &name = track_qv->GetName();
      auto qn_weight = track_qv->GetWeightVar().GetName() == "_Ones" ? "Ones" : track_qv->GetWeightVar().GetName();
      manager_.AddDetector(name,
                           DetectorType::TRACK,
                           track_qv->GetPhiVar().GetName(),
                           qn_weight,
                           track_qv->GetAxes(),
                           {1, 2},
                           track_qv->GetNormalization());
      Info(__func__, "Add track detector '%s'", name.c_str());
      SetCorrectionSteps(track_qv.operator*());
      for (const auto &cut : track_qv->GetCuts()) {//NOTE cannot apply cuts on more than 1 variable
        manager_.AddCutOnDetector(name, {cut.GetVariable().GetName().c_str()}, cut.GetFunction(), cut.GetDescription());
      }
    } else if (qvec_ptr->GetType() == Base::EQVectorType::CHANNEL) {
      auto channel_qv = std::dynamic_pointer_cast<Base::QVectorChannel>(qvec_ptr);
      const string name = channel_qv->GetName();
      auto qn_phi = name + "_" + channel_qv->GetPhiVar().GetName();
      auto qn_weight =
          channel_qv->GetWeightVar().GetName() == "_Ones" ? "Ones" : name + "_" + channel_qv->GetWeightVar().GetName();
      manager_.AddDetector(name,
                           DetectorType::CHANNEL,
                           qn_phi,
                           qn_weight,
                           {/* no axes to be passed */},
                           {1},
                           channel_qv->GetNormalization());
      Info(__func__, "Add channel detector '%s'", name.c_str());
      SetCorrectionSteps(channel_qv.operator*());
    } else if (qvec_ptr->GetType() == Base::EQVectorType::EVENT_PSI) {
      string name = qvec_ptr->GetName();
      string qn_phi = qvec_ptr->GetPhiVar().GetName();
      auto qn_weight = qvec_ptr->GetWeightVar().GetName() == "_Ones" ? "Ones" : qvec_ptr->GetWeightVar().GetName();
      manager_.AddDetector(name, DetectorType::CHANNEL, qn_phi, qn_weight, {}, {1, 2}, qvec_ptr->GetNormalization());
      Info(__func__, "Add event PSI '%s'", name.c_str());
      SetCorrectionSteps(qvec_ptr.operator*());
    }
  }

  AddQAHisto();

  //Initialization of framework
  manager_.InitializeOnNode();
  manager_.SetCurrentRunName("test");
}

void QnCorrectionTask::InitVariables() {
  // Add all needed variables
  short ivar{0}, ibranch{0};

  for (auto &entry : var_manager_->VarEntries()) {
    if (entry.GetNumberOfBranches() > 1) {
      auto &branches = entry.GetBranches();
      if (!std::all_of(branches.begin(), branches.end(), [](AnalysisTree::BranchReader *reader) {
        return reader->GetType() == AnalysisTree::DetType::kEventHeader;
      })) {
        throw std::runtime_error("More than one branch in one entry is allowed only if ALL of them EventHeader-s");
      }
    }

    if (entry.GetBranches()[0]->GetType() == AnalysisTree::DetType::kModule) {
      ibranch++;
      continue;
    }// Should be handled separately

    for (auto &var : entry.Variables()) {
      if (var.GetName() != "_Ones") {
        var.SetId(ivar);
        manager_.AddVariable(var.GetName(), var.GetId(), var.GetSize());
        //        var.Print();
        ivar += var.GetSize();
      }
    }

    auto type = entry.GetBranches()[0]->GetType();
    if (type != AnalysisTree::DetType::kEventHeader && type != AnalysisTree::DetType::kModule) {
      manager_.AddVariable(entry.GetBranches()[0]->GetName() + "_Filled", ivar, 1);
      is_filled_.insert(std::make_pair(ibranch, ivar));
      ivar++;
    }

    ibranch++;
  }

  for (auto &qvec : analysis_setup_->channel_qvectors_) {
    auto &phi = qvec->PhiVar();
    phi.SetId(ivar);
    manager_.AddVariable(qvec->GetName() + "_" + phi.GetName(), phi.GetId(), phi.GetSize());
    ivar += phi.GetSize();
    auto &weight = qvec->WeightVar();
    weight.SetId(ivar);
    manager_.AddVariable(qvec->GetName() + "_" + weight.GetName(), weight.GetId(), weight.GetSize());
    ivar += weight.GetSize();
  }

  for (const auto &event_var : analysis_setup_->GetEventVars()) {
    manager_.AddEventVariable(event_var.GetName());
  }
}
/**
* Main method. Executed every event
*/
void QnCorrectionTask::UserExec() {
  manager_.Reset();
  double *container = manager_.GetVariableContainer();

  for (const auto &entry : var_manager_->GetVarEntries()) {
    if (entry.GetBranches()[0]->GetType() == AnalysisTree::DetType::kEventHeader) {
      short ivar{0};
      for (const auto &var : entry.GetVariables()) {
        container[var.GetId()] = entry.GetValues().at(0)[ivar];
        ivar++;
      }
    }
  }
  for (const auto &qvec : analysis_setup_->channel_qvectors_) {
    const auto &phi = qvec->GetPhiVar();
    const auto &weight = qvec->GetWeightVar();
    const auto &branch = var_manager_->GetVarEntries().at(qvec->GetVarEntryId());
    int i_channel{0};
    for (int i : qvec->GetModuleIds()) {
      auto module_x = data_header_->GetModulePositions(0).GetChannel(i).GetX();
      auto module_y = data_header_->GetModulePositions(0).GetChannel(i).GetY();
      auto module_phi = data_header_->GetModulePhi(0, i); //TODO fix hardcoded 0
      container[phi.GetId() + i_channel] = module_phi;
      if (i < branch.GetValues().size()) {
        container[weight.GetId() + i_channel] = branch.GetValues()[i].at(0);//TODO fix hardcoded 0
      } else {
        container[weight.GetId() + i_channel] = 0.;
      }
      i_channel++;
    }
  }
  manager_.ProcessEvent();
  manager_.FillChannelDetectors();

  FillTracksQvectors();

  manager_.ProcessCorrections();
}

/**
* Fill the information from Tracks, Particles and Hits. We assume that Tracking Q-vectors are not constructed from
* Modules. Information from EventHeaders and Modules should be filled before.
*/
void QnCorrectionTask::FillTracksQvectors() {
  double *container = manager_.GetVariableContainer();
  short ibranch{0};
  for (const auto &entry : var_manager_->GetVarEntries()) {
    auto type = entry.GetBranches()[0]->GetType();
    if (type == AnalysisTree::DetType::kEventHeader || type == AnalysisTree::DetType::kModule) {
      ibranch++;
      continue;// skip EventHeader and ModuleDetectors
    }
    assert(entry.GetNumberOfBranches() == 1);
    const int n_channels = entry.GetValues().size();
    for (int i = 0; i < n_channels; ++i) {
      for (auto const &fill : is_filled_) {
        container[fill.second] = fill.first == ibranch ? 1. : -1.;// 1 for current, -1 for all others
      }
      short ivar{0};
      for (const auto &var : entry.GetVariables()) {
        container[var.GetId()] = entry.GetValues()[i].at(ivar);
        ivar++;
      }
      manager_.FillTrackingDetectors();
    }
    ibranch++;
  }
}

boost::program_options::options_description QnCorrectionTask::GetBoostOptions() {
  using namespace boost::program_options;
  options_description desc(GetName() + " options");
  desc.add_options()
      ("calibration-input-file",
       value(&in_calibration_file_name_)->default_value("correction_in.root"),
       "Input calibration file")
      ("yaml-config-file", value(&yaml_config_file_)->default_value("analysis-config.yml"), "Path to YAML config")
      ("yaml-config-name", value(&yaml_config_node_)->required(), "Name of YAML node")
      ("qa-file", value(&qa_file_name_)->default_value(""), "Produce dedicated file with QA");
  return desc;
}

/**
* Adding QA histograms to CorrectionManager
*/

void QnCorrectionTask::AddQAHisto() {
  for (auto q_tra : analysis_setup_->track_qvectors_) {
    for (const auto &qa : q_tra->GetQAHistograms()) {
      if (qa.axes.size() == 1) {
        manager_.AddHisto1D(q_tra->GetName(), qa.axes.at(0).GetQnAxis(),
                            qa.weight);
      } else if (qa.axes.size() == 2) {
        manager_.AddHisto2D(q_tra->GetName(), {qa.axes.at(0).GetQnAxis(), qa.axes.at(1).GetQnAxis()},
                            qa.weight);
      } else {
        throw std::runtime_error("QA histograms with more than 2 axis (or less than one) are not supported.");
      }
    }
  }

  for (const auto &q_ch : analysis_setup_->channel_qvectors_) {
    for (const auto &qa : q_ch->GetQAHistograms()) {
      if (qa.axes.size() == 1) {
        auto axis = qa.axes.at(0).GetQnAxis();
        axis.SetName(q_ch->GetName() + "_" + axis.Name());
        manager_.AddHisto1D(q_ch->GetName(), axis, qa.weight);
      } else {
        throw std::runtime_error("FIX ME");
      }
    }
  }

  for (const auto &q_psi : analysis_setup_->psi_qvectors_) {
    for (const auto &qa : q_psi->GetQAHistograms()) {
      if (qa.axes.size() == 1) {
        auto axis = qa.axes.at(0).GetQnAxis();
        manager_.AddHisto1D(q_psi->GetName(), axis, qa.weight);
      } else {
        throw std::runtime_error("FIX ME");
      }
    }
  }

  for (const auto &histo : analysis_setup_->qa_) {
    if (histo.axes.size() == 1) {
      manager_.AddEventHisto1D(histo.axes[0].GetQnAxis(), histo.weight);
    } else if (histo.axes.size() == 2) {
      manager_.AddEventHisto2D({histo.axes[0].GetQnAxis(), histo.axes[1].GetQnAxis()}, histo.weight);
    } else {
      throw std::runtime_error("QA histograms with more than 2 axis (or less than one) are not supported.");
    }
  }
}

void QnCorrectionTask::UserFinish() {
  manager_.Finalize();

  out_file_->cd();
  out_tree_->Write("tree");

  auto *correction_list = manager_.GetCorrectionList();
  auto *correction_qa_list = manager_.GetCorrectionQAList();
  correction_list->Write("CorrectionHistograms", TObject::kSingleKey);
  if (qa_file_name_.empty()) {
    Info(__func__, "Writing QA to output file...");
    correction_qa_list->Write("CorrectionQAHistograms", TObject::kSingleKey);
  }
  out_file_->Close();
  if (!qa_file_name_.empty()) {
    Info(__func__, "Writing QA to '%s'...", qa_file_name_.c_str());
    TFile qa_file(qa_file_name_.c_str(), "RECREATE");
    correction_qa_list->Write("CorrectionQAHistograms", TObject::kSingleKey);
  }
}
/**
* Set correction steps in a CorrectionManager for a given Q-vector
*/
void QnCorrectionTask::SetCorrectionSteps(const Base::QVector &qvec) {
  std::vector<Qn::QVector::CorrectionStep> correction_steps = {Qn::QVector::CorrectionStep::PLAIN};
  const std::string &name = qvec.GetName();

  for (const auto &correction : qvec.GetCorrections()) {
    if (correction->IsA() == Qn::Recentering::Class()) {
      auto recentering = std::dynamic_pointer_cast<Qn::Recentering>(correction);
      correction_steps.emplace_back(Qn::QVector::CorrectionStep::RECENTERED);
      manager_.AddCorrectionOnQnVector(name, recentering.operator*());
      Info(__func__, "Add Recentering to '%s'", name.c_str());
    } else if (correction->IsA() == Qn::TwistAndRescale::Class()) {
      auto twist_and_rescale = std::dynamic_pointer_cast<Qn::TwistAndRescale>(correction);
      correction_steps.emplace_back(Qn::QVector::CorrectionStep::TWIST);
      correction_steps.emplace_back(Qn::QVector::CorrectionStep::RESCALED);
      manager_.AddCorrectionOnQnVector(name, twist_and_rescale.operator*());
      Info(__func__, "Add Twist-And-Rescale to '%s'", name.c_str());
    }
  }

  manager_.SetOutputQVectors(name, correction_steps);
}

}// namespace Qn
