//=== TypeChecksCmpOpt.h - Remove runtime type checks for ptrs used in cmp ===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass removes type checks that are on values used in compares
//
//===----------------------------------------------------------------------===//

#ifndef TYPE_CHECKS_CMP_OPT_H
#define TYPE_CHECKS_CMP_OPT_H

#include "assistDS/TypeAnalysis.h"

#include "llvm/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Support/CallSite.h"

#include <set>

namespace llvm {

class Type;
class Value;

class TypeChecksCmpOpt : public ModulePass {

private:

  std::set<Instruction *> toDelete;

public:
  static char ID;
  TypeChecksCmpOpt() : ModulePass(&ID) {}
  virtual bool runOnModule(Module &M);

};

} // End llvm namespace

#endif
