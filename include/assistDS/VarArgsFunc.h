//===--- VarArgsFunc.cpp - Simplify calls to bitcasted const funcs --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Convert calls of type
// call(bitcast F to (...)*) ()
// to
// call F()
// if the number and types of arguments passed matches.
//===----------------------------------------------------------------------===//

#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"

namespace llvm {
  //
  // Class: VarArgsFunc
  //
  class VarArgsFunc : public ModulePass {
  public:
    static char ID;
    VarArgsFunc() : ModulePass(&ID) {}
    virtual bool runOnModule(Module& M);
  };
}

