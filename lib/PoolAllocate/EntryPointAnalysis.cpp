//===-- EntryPointAnalysis.cpp - Entry point Finding Pass -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is a general way of finding entry points in a system.  Simple programs
// will use the main version.  Libraries and OS kernels can have more
// specialized versions.  This is done as an analysis group to allow more
// convinient opt invocations.
//
//===----------------------------------------------------------------------===//

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Function.h"

#include "EntryPointAnalysis.h"

using namespace llvm;

EntryPointAnalysis::~EntryPointAnalysis() {}
char EntryPointAnalysis::ID;

namespace {


static RegisterAnalysisGroup<EntryPointAnalysis> A("Entry Point Analysis");

class MainEntryPointAnalysis : public ImmutablePass, public EntryPointAnalysis {
public:
  static char ID;

  MainEntryPointAnalysis() : ImmutablePass(&ID) { }

  bool isEntryPoint(const llvm::Function* F) const {
    return !F->isDeclaration()
            && F->hasExternalLinkage()
            && F->hasName() && F->getName() == "main";
  }
};

char MainEntryPointAnalysis::ID;
RegisterPass<MainEntryPointAnalysis> B("epa_main", "Identify Main");
RegisterAnalysisGroup<EntryPointAnalysis, true> C(B);



}

