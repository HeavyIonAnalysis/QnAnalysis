//
// Created by eugene on 10/01/2022.
//

#include <Correlation.hpp>
#include <TFile.h>

using namespace ::C4::CorrelationOps;
using namespace ::C4::TensorOps;
using namespace std;

auto r1_3sub = [](
    const Enumeration<std::string> &references,
    const Enumeration<EComponent> &components,
    TDirectory *d) {
  assert(references.size() == 3);
  auto references_1 = references.clone(references.getName() + "_tmp1");
  auto references_2 = references.clone(references.getName() + "_tmp2");

  auto q_a = qt(references, 1u, components);
  auto q_b = qt(references_1, 1u, components);
  auto q_c = qt(references_2, 1u, components);

  auto r1_big_tensor = sqrt(Value(2.0) * ct(d, q_a, q_b) * ct(d, q_a, q_c) / ct(d, q_b, q_c));

  return Tensor({
                    {references.getName(), references.size()},
                    {components.getName(), components.size()}
                },
                [=](const TensorIndex &index) {
                  const std::vector<TensorLinearIndex> tmp1_ind = {/* 0 */ 1,  /* 1 */ 2, /* 2 */ 0};
                  const std::vector<TensorLinearIndex> tmp2_ind = {/* 0 */ 2,  /* 1 */ 0, /* 2 */ 1};
                  const TensorIndex big_index{
                      {references.getName(), index.at(references.getName())},
                      {components.getName(), index.at(components.getName())},
                      {references_1.getName(), tmp1_ind.at(index.at(references.getName()))},
                      {references_2.getName(), tmp2_ind.at(index.at(references.getName()))},
                  };
                  return r1_big_tensor.at(big_index);
                });
};

int main() {

  auto input_file = TFile::Open("correlations.root", "read");
  if (!input_file) {
    return 1;
  }

  auto uq_dir = (TDirectory *) input_file->Get("uQ");
  auto qq_dir = (TDirectory *) input_file->Get("QQ");

  uq_dir->ls();

  auto enum_observable_particle = enumerate<std::string>("observable_particle", {
      "protons_RESCALED",
      "pion_neg_RESCALED",
      "pion_pos_RESCALED"});
  auto enum_psd_reference = enumerate<std::string>("psd_reference", {
      "psd1_RECENTERED",
      "psd2_RECENTERED",
      "psd3_RECENTERED"
  });
  auto enum_sp_component = enumerate("sp_component", {
      EComponent::X,
      EComponent::Y
  });
  auto enum_sp_component_inv = enumerate("sp_component", {
    EComponent::Y,
    EComponent::X
  });

  auto obs_uq = ct(uq_dir,
                   qt(enum_observable_particle, 1u, enum_sp_component),
                   qt(enum_psd_reference, 1u, enum_sp_component));

  for (auto && uq : obs_uq) {
    auto lazy_result = uq();

    try {
      auto result = lazy_result.value();

      cout << lazy_result.nameInFile() << endl;
      result.Print();
      cout << endl;
    } catch (Correlation::CorrelationNotFoundException&) {}
  }


  auto obs_uq_crosscomp = ct(uq_dir,
                   qt(enum_observable_particle, 1u, enum_sp_component_inv),
                   qt(enum_psd_reference, 1u, enum_sp_component));
  auto r1_3sub_t = r1_3sub(enum_psd_reference, enum_sp_component, qq_dir);


  cout << "Resolution (three sub events)" << endl;

  {
    auto proc_resolution_file = TFile::Open("proc_resolution.root", "recreate");
    for (auto &&r1 : r1_3sub_t) {
      auto lazy_result = r1();
      cout << lazy_result << endl;

      try {
        auto result = lazy_result.value();
        result.Print();

        std::stringstream obj_name_stream;
        obj_name_stream
        << "3sub__"
        << enum_psd_reference.at(r1.index) << "__"
        << "SP_" << (enum_sp_component.at(r1.index) == EComponent::X ? "X" : "Y");
        auto obj_name = obj_name_stream.str();
        proc_resolution_file->WriteObject(&result, obj_name.c_str());
      } catch (Correlation::CorrelationNotFoundException &) {
        cout << "Not found. Skipping..." << endl;
      }
      cout << endl;
    }
    proc_resolution_file->Close();
  }


  cout << "v1 (3sub)..." << endl;
  {
    auto v1_3sub = Value(2.0) * obs_uq / r1_3sub_t;
    auto proc_v1_file = TFile::Open("proc_v1.root", "recreate");
    for (auto &&v1 : v1_3sub) {
      auto lazy_result = v1();
      cout << lazy_result << endl;

      try {
        auto result = lazy_result.value();
        result.Print();

        std::stringstream obj_name_stream;
        obj_name_stream
        << enum_observable_particle.at(v1.index) << "__"
        << enum_psd_reference.at(v1.index) << "__"
        << "3sub__"
        << "SP_" << (enum_sp_component.at(v1.index) == EComponent::X ? "X" : "Y");
        auto obj_name = obj_name_stream.str();
        proc_v1_file->WriteObject(&result, obj_name.c_str());
      } catch (Correlation::CorrelationNotFoundException &) {
        cout << "Not found. Skipping..." << endl;
      }
      cout << endl;
    }
    proc_v1_file->Close();
  }

  cout << "c1 (3sub)..." << endl;
  {
    auto c1_3sub = Value(2.0) * obs_uq_crosscomp / r1_3sub_t;
    auto proc_c1_file = TFile::Open("proc_c1.root", "recreate");
    for (auto &&v1 : c1_3sub) {
      auto lazy_result = v1();
      cout << lazy_result << endl;

      try {
        auto result = lazy_result.value();
        result.Print();

        std::stringstream obj_name_stream;
        obj_name_stream
        << enum_observable_particle.at(v1.index) << "__"
        << enum_psd_reference.at(v1.index) << "__"
        << "3sub__"
        << "SP_" << (enum_sp_component_inv.at(v1.index) == EComponent::X ? "X" : "Y");
        auto obj_name = obj_name_stream.str();
        proc_c1_file->WriteObject(&result, obj_name.c_str());
      } catch (Correlation::CorrelationNotFoundException &) {
        cout << "Not found. Skipping..." << endl;
      }
      cout << endl;
    }
    proc_c1_file->Close();
  }





  return 0;
}