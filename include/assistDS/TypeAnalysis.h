//===-- TypeAnalysis.h - Deciphers Types of loads and stores --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This code defines a pass which clones functions which could potentially be
// used in indirect function calls.
//
//===----------------------------------------------------------------------===//

#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"

namespace llvm {
  //
  // Class: TypeAnalysis 
  //
  // Description:
  //  Implement an LLVM pass that analyses a file to say what
  //  the type of a load/store instruction is.
  //
  class TypeAnalysis : public ModulePass {
  public:
    static char ID;
    TypeAnalysis() : ModulePass(&ID) {}
    virtual bool runOnModule(Module& M);
    virtual void getAnalysisUsage(AnalysisUsage &Info) const;

    const Type *getType(LoadInst *);
    const Type *getType(StoreInst *);
    bool isCopyingLoad(LoadInst *);
    bool isCopyingStore(StoreInst *);
    Value *getStoreSource(StoreInst *SI);
  };
}

