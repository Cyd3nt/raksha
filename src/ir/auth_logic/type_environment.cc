/*
 * Copyright 2022 Google LLC.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/ir/auth_logic/type_environment.h"

#include "src/ir/auth_logic/is_name_constant.h"

namespace raksha::ir::auth_logic {

namespace {

// This visitor aids in the construction of the TypeEnvironment.
// It populates a mapping (flat_hash_map) of constant (literal)
// names to their types by looking at the relations to which the literals
// are applied and declarations of those relations (including the types
// of the parameters where they are applied). It takes as input
// a DeclarationEnvironment.
// This is a nested class because it needs to access the private
// fields of the TypeEnvironment in the constructor implementation.


class TypeEnvironmentGenerationVisitor
    : public AuthLogicAstTraversingVisitor<TypeEnvironmentGenerationVisitor> {
 public:
using LiteralTypeMapType =
    absl::flat_hash_map<std::string, datalog::ArgumentType>;
  TypeEnvironmentGenerationVisitor(DeclarationEnvironment decl_env)
      : decl_env_(decl_env), literal_type_map_({}) {}

  LiteralTypeMapType literal_type_map() { return literal_type_map_; }

 private:
  void AddTyping(absl::string_view arg_name, datalog::ArgumentType arg_type);
  Unit PreVisit(const Principal& principal);
  Unit Visit(const datalog::Predicate& pred);
  DeclarationEnvironment decl_env_;
  LiteralTypeMapType literal_type_map_;
};

void TypeEnvironmentGenerationVisitor::AddTyping(
    absl::string_view arg_name, datalog::ArgumentType arg_type) {
  if (!IsNameConstant(arg_name)) {
    // This pass is only meant to find typings for constants,
    // so do nothing if this argument is not a constant.
    return;
  }
  auto find_result = literal_type_map_.find(arg_name);
  if (find_result == literal_type_map_.end()) {
    // There is no entry for this literal in the map, add it:
    literal_type_map_.insert({std::string(arg_name), arg_type});
  } else {
    // There is already an entry for this literal in the map,
    // check that it has the same type:
    CHECK(find_result->second == arg_type)
        << "Type error for constant: " << arg_name;
  }

}

// Insert a typing for constant value (e.g. "SomeArgument" in quotes
// applied to a relation, or a constant principal in any context where
// principals may be applied.
// - If no typing for this constant already exists, the typing is added
// - If a typing for this constant exists but it is the same type as the new
// one, nothing happens
// - If a typing for this constant exists, and it is a different type from
// the new one, an error is thrown.
Unit TypeEnvironmentGenerationVisitor::PreVisit(const Principal& principal) {
  AddTyping(principal.name(), datalog::ArgumentType::MakePrincipalType());
  return Unit();
}

Unit TypeEnvironmentGenerationVisitor::Visit(const datalog::Predicate& pred) {
  if (pred.IsNumericOperator()) {
    // This is part of the workaround for handling numeric comparisons
    // given that the operators use the same AST nodes as predicates.
    for (auto arg : pred.args()) {
      AddTyping(arg, datalog::ArgumentType::MakeNumberType());
    }
    return Unit();
  } else {
    // This is the case where this is a normal predicate rather
    // than a numeric operator
    datalog::RelationDeclaration decl =
        decl_env_.GetDeclarationOrFatal(pred.name());
    // The relation declarations give the types for each position in the
    // predicate. For the xth argument in the predicate, we want to
    // assign it the type for the xth parameter in the relation declaration.
    // So to do this, we iterate through the list of arguments in the
    // predicate and the list of parameters in the declaration using
    // the indices of the arguments iterator of the predicate.
    for (size_t i = 0; i < pred.args().size(); i++) {
      AddTyping(pred.args()[i], decl.arguments()[i].argument_type());
    }
    return Unit();
  }
}

}  // namespace

TypeEnvironment::TypeEnvironment(DeclarationEnvironment decl_env,
                                 const Program& prog) {
  TypeEnvironmentGenerationVisitor type_gen_visitor(decl_env);
  prog.Accept(type_gen_visitor);
  literal_type_map_ = type_gen_visitor.literal_type_map();
}

datalog::ArgumentType TypeEnvironment::GetTypingOrFatal(
    absl::string_view argument_name) {
  auto find_result = literal_type_map_.find(argument_name);
  CHECK(find_result != literal_type_map_.end())
      << "Could not find typing for argument " << argument_name;
  return find_result->second;
}

}  // namespace raksha::ir::auth_logic
