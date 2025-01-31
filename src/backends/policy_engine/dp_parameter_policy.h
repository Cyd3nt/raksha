//-----------------------------------------------------------------------------
// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//----------------------------------------------------------------------------
#ifndef SRC_BACKENDS_POLICY_ENGINE_DP_PARAMETER_POLICY_H_
#define SRC_BACKENDS_POLICY_ENGINE_DP_PARAMETER_POLICY_H_

#include <filesystem>
#include <optional>

#include "absl/strings/str_format.h"
#include "src/backends/policy_engine/policy.h"

namespace raksha::backends::policy_engine {

// A policy where the contents of the policy are just differential privacy
// parameters. For now, we only send over Epsilon and Delta, but we can extend
// this to send more in the future.
class DpParameterPolicy : public Policy {
 public:
  // Note: The SQL frontend hands us these parameters as floating point
  // numbers, so we do eventually want to represent these as `double`s. But
  // the IR parser hasn't landed floating point numbers yet, so using
  // `uint64_t`s for now.
  using DpParameterNumericType = uint64_t;

  explicit DpParameterPolicy(DpParameterNumericType epsilon,
                             DpParameterNumericType delta)
      : epsilon_(epsilon), delta_(delta) {}

  // Always use the checker compiled from the DP policy verifier interface.
  std::string GetPolicyAnalysisCheckerName() const override {
    return "dp_policy_verifier_cxx";
  }

  // This policy always populates the `isDPParameter` relation.
  std::optional<std::string> GetPolicyFactName() const override {
    return "isDPParameter";
  }

  std::optional<std::string> GetPolicyString() const override {
    return absl::StrFormat(
        R"($EpsilonValue(%lu)
$DeltaValue(%lu))",
        epsilon_, delta_);
  }

 private:
  // The epsilon differential privacy parameter.
  DpParameterNumericType epsilon_;
  // The delta differential privacy parameter.
  DpParameterNumericType delta_;
};

}  // namespace raksha::backends::policy_engine

#endif
