#include "QnCorrectionTask.hpp"

#include <iostream>
#include <memory>

#include <AnalysisTree/DataHeader.hpp>
#include <AnalysisTree/TreeReader.hpp>

#include <QnAnalysisBase/QVector.hpp>
#include <QnAnalysisBase/AnalysisSetup.hpp>
#include <QnAnalysisConfig/Config.hpp>

#include <QnAnalysisCorrect/ATVarManagerTask.hpp>

namespace Qn::Analysis::Correction {

TASK_IMPL(QnCorrectionTask)

using std::string;
using std::vector;

std::string GetATFieldName(const AnalysisTree::Variable &v) {
  return v.GetFields()[0].GetBranchName() + "/" + v.GetFields()[0].GetName();
}

std::string GetQnFieldName(const AnalysisTree::Variable &v) {
  if (v.GetName() == "_Ones") {
    return "Ones";
  }
  return v.GetName();
}

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
      const string &q_vector_name = track_qv->GetName();
      const string target_branch_name = track_qv->GetPhiVar().GetFields()[0].GetBranchName();

      auto loop_ctx_branch_it = find_if(begin(track_loop_contexts_),
                                        end(track_loop_contexts_),
                                        [target_branch_name](const MappingContext &ctx) {
                                          return target_branch_name == ctx.GetBranchName();
                                        });
      if (loop_ctx_branch_it == end(track_loop_contexts_)) {
        auto lock_variable = AddQnVariable(target_branch_name + "_Filled")[0];
        MappingContext loop_ctx(GetInBranch(target_branch_name), lock_variable);
        track_loop_contexts_.emplace_back(loop_ctx);
        loop_ctx_branch_it = track_loop_contexts_.end() - 1;
      }

      for (auto &variable : track_qv->GetListOfVariables()) {
        auto qn_field_name = GetQnFieldName(variable);
        if (qn_field_name == "Ones")
          continue;
        auto qn_field = FindFirstQnVariableByName(qn_field_name);
        if (!qn_field) {
          qn_field = AddQnVariable(qn_field_name, 1)[0];
        }
        auto at_field_name = GetATFieldName(variable);
        auto at_field = GetVar(at_field_name);
        loop_ctx_branch_it->AddMapping(at_field, qn_field);
      }

      manager_.AddDetector(q_vector_name,
                           DetectorType::TRACK,
                           GetQnFieldName(track_qv->GetPhiVar()),
                           GetQnFieldName(track_qv->GetWeightVar()),
                           track_qv->GetAxes(),
                           {1, 2},
                           track_qv->GetNormalization());
      Info(__func__, "Add track detector '%s'", q_vector_name.c_str());
      SetCorrectionSteps(track_qv.operator*());

      manager_.AddCutOnDetector(
          q_vector_name,
          {loop_ctx_branch_it->lock_qn_variable->name.c_str()},
          [](const double lock) {
            return lock > 0;
          }, "is_filled");

      for (const auto &cut : track_qv->GetCuts()) {//NOTE cannot apply cuts on more than 1 variable
        manager_.AddCutOnDetector(q_vector_name,
                                  {cut.GetVariable().GetName().c_str()},
                                  cut.GetFunction(),
                                  cut.GetDescription());
      }

    } else if (qvec_ptr->GetType() == Base::EQVectorType::CHANNEL) {
      auto channel_qv = std::dynamic_pointer_cast<Base::QVectorChannel>(qvec_ptr);
      auto q_vector_name = channel_qv->GetName();

      auto qn_phi_name = q_vector_name + "_" + GetQnFieldName(channel_qv->GetPhiVar());

      { /* phi variable */
        if (FindFirstQnVariableByName(qn_phi_name)) {
          throw std::runtime_error("Qn field '" + qn_phi_name + "' already defined");
        }
        auto qn_phi_fields = AddQnVariable(qn_phi_name, channel_qv->GetModuleIds().size());
        size_t i_qn_channel = 0;
        for (int module_id : channel_qv->GetModuleIds()) {
          auto data_header_phi_fn = [this, module_id]() {
            return this->data_header_->GetModulePhi(0, module_id); //TODO fix hardcoded 0
          };
          auto data_header_phi_vs = std::make_shared<FunctionValueSourceImpl>(data_header_phi_fn);
          event_var_mapping_.emplace_back(data_header_phi_vs, qn_phi_fields[i_qn_channel]);
          ++i_qn_channel;
        }
      }

      auto qn_weight_name = channel_qv->GetWeightVar().GetName() == "_Ones" ? "Ones" : q_vector_name + "_"
          + channel_qv->GetWeightVar().GetName();
      if (qn_weight_name != "Ones") {
        if (FindFirstQnVariableByName(qn_weight_name)) {
          throw std::runtime_error("Qn field '" + qn_weight_name + "' already defined");
        }
        auto qn_weight_fields = AddQnVariable(qn_weight_name, channel_qv->GetModuleIds().size());
        size_t i_qn_channel = 0;
        for (int module_id : channel_qv->GetModuleIds()) {
          auto ati2_variable = GetVar(GetATFieldName(channel_qv->GetWeightVar()));
          auto ati2_source = std::make_shared<ATI2ValueSourceImpl>(ati2_variable, module_id);
          event_var_mapping_.emplace_back(ati2_source, qn_weight_fields[i_qn_channel]);
          ati2_event_sources_.emplace_back(ati2_source);
          ++i_qn_channel;
        }

      }

      manager_.AddDetector(q_vector_name,
                           DetectorType::CHANNEL,
                           qn_phi_name,
                           qn_weight_name,
                           {/* no axes to be passed */},
                           {1},
                           channel_qv->GetNormalization());
      Info(__func__, "Add channel detector '%s'", q_vector_name.c_str());
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
        auto added_variables = AddQnVariable(var.GetName(), var.GetSize());
        var.SetId(added_variables.front()->idx);
        //        var.Print();
        ivar += var.GetSize();
      }
    }

    auto type = entry.GetBranches()[0]->GetType();
    if (type != AnalysisTree::DetType::kEventHeader && type != AnalysisTree::DetType::kModule) {
//      auto added_variable = AddQnVariable(entry.GetBranches()[0]->GetName() + "_Filled", 1);
//      is_filled_.insert(std::make_pair(ibranch, added_variable.front()->idx));
//      ivar++;
    }

    ibranch++;
  }

//  for (auto &qvec : analysis_setup_->channel_qvectors_) {
//    auto &phi = qvec->PhiVar();
//    phi.SetId(ivar);
//    manager_.AddVariable(qvec->GetName() + "_" + phi.GetName(), phi.GetId(), phi.GetSize());
//    ivar += phi.GetSize();
//    auto &weight = qvec->WeightVar();
//    weight.SetId(ivar);
//    manager_.AddVariable(qvec->GetName() + "_" + weight.GetName(), weight.GetId(), weight.GetSize());
//    ivar += weight.GetSize();
//  }

  for (const auto &event_var : analysis_setup_->GetEventVars()) {
    manager_.AddEventVariable(event_var.GetName());
  }
}
/**
* Main method. Executed every event
*/
void QnCorrectionTask::UserExec() {
  manager_.Reset();

  {
    double *container = manager_.GetVariableContainer();

    for (auto &v : qn_variables_) {
      v->Reset();
      v->data = container;
    }
  }


  for (auto &ati2_source : ati2_event_sources_) {
    ati2_source->Update();
  }

  for (auto &[source, sink] : event_var_mapping_) {
    source.Notify();
    sink.Reset();
  }

  for (auto &[source, sink] : event_var_mapping_) {
    auto value = source.Value();
    sink.AssignValue(value);
  }

  manager_.ProcessEvent();
  manager_.FillChannelDetectors();

  for (auto &track_loop : track_loop_contexts_) {
    if (track_loop.lock_qn_variable) {
      track_loop.lock_qn_variable->Reset();
      track_loop.lock_qn_variable->AssignValue(1.0);
    }
    for (auto &channel : track_loop.branch_ptr->Loop()) {
      /* renew ATI2 sources */
      for (auto &source : track_loop.ati2_sources_) {
        source->Update(channel);
      }
      for (auto &[_, sink] : track_loop.mappings_) {
        sink.Reset();
      }
      for (auto &[source, sink] : track_loop.mappings_) {
        source.Notify();
        sink.AssignValue(source.Value());
      }
      manager_.FillTrackingDetectors();
    }
    if (track_loop.lock_qn_variable) {
      track_loop.lock_qn_variable->Reset();
      track_loop.lock_qn_variable->AssignValue(-1.0);
    }
  }

  manager_.ProcessCorrections();
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
