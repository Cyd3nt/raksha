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

#include "src/backends/policy_engine/souffle/souffle_policy_checker.h"

#include <cstdlib>

#include "souffle/SouffleInterface.h"
#include "src/backends/policy_engine/policy.h"
#include "src/backends/policy_engine/souffle/datalog_lowering_visitor.h"
#include "src/backends/policy_engine/souffle/utils.h"
#include "src/common/utils/filesystem.h"
#include "src/ir/module.h"

namespace raksha::backends::policy_engine {

// Disambiguate "souffle" and "::souffle" with using aliases.
// These are from the Souffle project and have a top-level namespace of
// "::souffle".
using Relation = ::souffle::Relation;
using SouffleProgram = ::souffle::SouffleProgram;
using ProgramFactory = ::souffle::ProgramFactory;

// These are from Raksha's "souffle" subpackage.
using DatalogLoweringVisitor = souffle::DatalogLoweringVisitor;

static constexpr char kViolatesPolicyRelation[] = "violatesPolicy";

static bool IsModulePolicyCompliantHelper(
    const ir::Module& module, const Policy& policy,
    const std::filesystem::path& facts_directory) {
  // Output the facts contained in the policy if they exist.
  if (std::optional<std::string> optional_policy_fact_name =
          policy.GetPolicyFactName()) {
    std::optional<std::string> optional_policy_string =
        policy.GetPolicyString();
    CHECK(optional_policy_string);
    absl::Status output_policy_status = souffle::WriteFactsStringToFactsFile(
        facts_directory, *optional_policy_fact_name, *optional_policy_string);
    CHECK(output_policy_status.ok())
        << "Unexpected error while outputting policy: " << output_policy_status;
  }

  DatalogLoweringVisitor datalog_lowering_visitor;
  module.Accept(datalog_lowering_visitor);
  absl::Status output_module_status =
      datalog_lowering_visitor.datalog_facts().DumpFactsToDirectory(
          facts_directory, {});
  CHECK(output_module_status.ok())
      << "Unexpected error while outputting module: " << output_module_status;

  std::unique_ptr<SouffleProgram> program(
      CHECK_NOTNULL(ProgramFactory::newInstance(
          std::string(policy.GetPolicyAnalysisCheckerName()))));

  // If we allow Souffle to set up its signal handler, it can cause issues when
  // we call via JNI. See b/245620786 for a circumstance where this occurred.
  int64_t setenv_result = setenv("SOUFFLE_ALLOW_SIGNALS", "NO", 1);
  CHECK(setenv_result == 0) << "Could not set `SOUFFLE_ALLOW_SIGNALS";
  program->loadAll(facts_directory.string());
  program->run();
  Relation* hasPolicyViolation =
      CHECK_NOTNULL(program->getRelation(kViolatesPolicyRelation));

  return hasPolicyViolation->size() == 0;
}

bool SoufflePolicyChecker::IsModulePolicyCompliant(const ir::Module& module,
                                                   const Policy& policy) const {
  absl::StatusOr<std::filesystem::path> temp_dir =
      common::utils::CreateTemporaryDirectory();
  if (!temp_dir.ok()) {
    LOG(ERROR) << absl::StrFormat(
        "Could not create temporary directory for validating module: `%s`",
        temp_dir.status().ToString());
    return false;
  }
  bool result = IsModulePolicyCompliantHelper(module, policy, *temp_dir);
  std::filesystem::remove_all(*temp_dir);
  return result;
}

}  // namespace raksha::backends::policy_engine
