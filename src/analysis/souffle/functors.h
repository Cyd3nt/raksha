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
#ifndef THIRD_PARTY_RAKSHA_SRC_ANALYSIS_SOUFFLE_FUNCTORS_H_
#define THIRD_PARTY_RAKSHA_SRC_ANALYSIS_SOUFFLE_FUNCTORS_H_

#include "souffle/RecordTable.h"
#include "souffle/SymbolTable.h"
souffle::RamDomain log_wrapper_cxx(souffle::SymbolTable *symbolTable,
                                   souffle::RecordTable *recordTable,
                                   souffle::RamDomain n);

#endif  // THIRD_PARTY_RAKSHA_SRC_ANALYSIS_SOUFFLE_FUNCTORS_H_
