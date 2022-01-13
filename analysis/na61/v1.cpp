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
  using namespace ::C4::CorrelationOps;
  using namespace ::C4::TensorOps;
  using namespace ::C4::LazyOps;

  assert(references.size() == 3);

  auto q_a = qt(references[0], 1u, components);
  auto q_b = qt(references[1], 1u, components);
  auto q_c = qt(references[2], 1u, components);

  return Tensor({
                    {references.getName(), references.size()},
                    {components.getName(), components.size()}
                },
                [=](const TensorIndex &index) {
                  auto display_name =
                      std::string("R_{1,") + (components[index] == EComponent::X ? "X" : "Y") + "} 3 subevents " + "("
                          + references[index] + ")";
                  auto r1_fct = [d] (auto && q1, auto && q2, auto && q3) {
                    return sqrt(Value(2.0) * ct(d, q1, q2) * ct(d, q3, q1) / ct(d, q2, q3));
                  };
                  if (index.at(references.getName()) == 0) {
                    return r1_fct(q_a, q_b, q_c).at(index).asFunction(display_name);
                  } else if (index.at(references.getName()) == 1) {
                    return r1_fct(q_b, q_c, q_a).at(index).asFunction(display_name);
                  } else if (index.at(references.getName()) == 2) {
                    return r1_fct(q_c, q_a, q_b).at(index).asFunction(display_name);
                  } else {
                    throw std::runtime_error("3sub: unknown reference");
                  }
                });
};

auto r1_4sub = [](
    const Enumeration<std::string> &references,
    const Enumeration<EComponent> &components,
    TDirectory *d) {
  using namespace ::C4::CorrelationOps;
  using namespace ::C4::TensorOps;
  using namespace ::C4::LazyOps;
  assert(references.size() == 4);

  auto q_a = qt(references.at(0), 1u, components);
  auto q_b = qt(references.at(1), 1u, components);
  auto q_c = qt(references.at(2), 1u, components);
  auto q_t = qt(references.at(3), 1u, components);

  return Tensor({
                    {references.getName(), 3},
                    {components.getName(), components.size()}
                }, [=](const TensorIndex &index) {
    auto display_name_str = (std::stringstream() << "R_{1," << components.at(index) << "} " <<
                                                 "4 subevents " << "(" << references.at(index) << ")").str();
    if (index.at(references.getName()) == 0) {
      return sqrt(Value(2.0) * ct(d, q_a, q_c) * ct(d, q_t, q_a) / ct(d, q_t, q_c))
          .at(index)
          .asFunction(display_name_str);
    } else if (index.at(references.getName()) == 1) {
      auto rt = sqrt(Value(2.0) * ct(d, q_t, q_a) * ct(d, q_t, q_c) / ct(d, q_a, q_c));
      return (Value(2.0) * ct(d, q_t, q_b) / rt)
          .at(index)
          .asFunction(display_name_str);
    } else if (index.at(references.getName()) == 2) {
      return sqrt(Value(2.0) * ct(d, q_a, q_c) * ct(d, q_t, q_c) / ct(d, q_t, q_a))
          .at(index)
          .asFunction(display_name_str);
    } else {
      throw std::runtime_error("4sub: invalid reference");
    }
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
  auto enum_4sub_reference = enumerate<std::string>("psd_reference", {
      "psd1_RECENTERED",
      "psd2_RECENTERED",
      "psd3_RECENTERED",
      "4sub_ref_opt2_RESCALED"
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
  auto obs_uq_crosscomp = ct(uq_dir,
                             qt(enum_observable_particle, 1u, enum_sp_component_inv),
                             qt(enum_psd_reference, 1u, enum_sp_component));

  auto r1_3sub_tensor = r1_3sub(enum_psd_reference, enum_sp_component, qq_dir);
  auto r1_4sub_tensor = r1_4sub(enum_4sub_reference, enum_sp_component, qq_dir);
  auto r1_tensor = stack_tensors("resolution_method", {r1_3sub_tensor, r1_4sub_tensor});
  auto v1_4sub_tensor = Value(2.0) * obs_uq / r1_tensor;

  auto enum_resolution = enumerate<std::string>("resolution_method", {"3sub", "4sub_opt2"});

  cout << "Resolution..." << endl;
  {
    auto proc_resolution_file = TFile::Open("proc_resolution.root", "recreate");
    for (auto &&r1 : r1_tensor) {
      auto lazy_result = r1();
      cout << lazy_result << endl;

      try {
        auto result = lazy_result.value();
        result.Print();

        std::stringstream obj_name_stream;
        obj_name_stream
            << enum_resolution.at(r1.index) << "__"
            << enum_psd_reference.at(r1.index) << "__"
            << "SP_" << enum_sp_component[r1.index];
        auto obj_name = obj_name_stream.str();
        proc_resolution_file->WriteObject(&result, obj_name.c_str());
      } catch (Correlation::CorrelationNotFoundException &) {
        cout << "Not found. Skipping..." << endl;
      }
      cout << endl;
    }
    proc_resolution_file->Close();
  }

  cout << "v1 (ALL)..." << endl;
  {
    auto proc_v1_file = TFile::Open("proc_v1.root", "recreate");
    for (auto &&v1 : v1_4sub_tensor) {
      auto lazy_result = v1();
      cout << lazy_result << endl;

      try {
        auto result = lazy_result.value();
        result.Print();

        std::stringstream obj_name_stream;
        obj_name_stream
            << enum_observable_particle.at(v1.index) << "__"
            << enum_psd_reference.at(v1.index) << "__"
            << enum_resolution.at(v1.index) << "__"
            << "SP_" << enum_sp_component.at(v1.index);
        auto obj_name = obj_name_stream.str();
        proc_v1_file->WriteObject(&result, obj_name.c_str());
      } catch (Correlation::CorrelationNotFoundException &) {
        cout << "Not found. Skipping..." << endl;
      }

      cout << endl;
    }
    proc_v1_file->Close();
  }

  cout << "c1 (4sub)..." << endl;
  {
    auto c1_4sub_tensor = Value(2.0) * obs_uq_crosscomp / r1_4sub_tensor;
    auto proc_c1_file = TFile::Open("proc_c1.root", "recreate");
    for (auto &&v1 : c1_4sub_tensor) {
      auto lazy_result = v1();
      cout << lazy_result << endl;

      try {
        auto result = lazy_result.value();
        result.Print();

        std::stringstream obj_name_stream;
        obj_name_stream
            << enum_observable_particle.at(v1.index) << "__"
            << enum_psd_reference.at(v1.index) << "__"
            << "4sub__"
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