//===- Devirt.cpp - Devirtualize using the sig match intrinsic in llva ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "devirt"

#include "assistDS/Devirt.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/Statistic.h"

#include <iostream>
#include <algorithm>
#include <iterator>

using namespace llvm;

// Pass ID variable
char Devirtualize::ID = 0;

// Pass statistics
STATISTIC(FuncAdded, "Number of bounce functions added");
STATISTIC(CSConvert, "Number of call sites converted");

// Pass registration
RegisterPass<Devirtualize>
X ("devirt", "Devirtualize indirect function calls");

//
// Function: castTo()
//
// Description:
//  Given an LLVM value, insert a cast instruction to make it a given type.
//
static inline Value *
castTo (Value * V, const Type * Ty, std::string Name, Instruction * InsertPt) {
  //
  // Don't bother creating a cast if it's already the correct type.
  //
  if (V->getType() == Ty)
    return V;

  //
  // If it's a constant, just create a constant expression.
  //
  if (Constant * C = dyn_cast<Constant>(V)) {
    Constant * CE = ConstantExpr::getZExtOrBitCast (C, Ty);
    return CE;
  }

  //
  // Otherwise, insert a cast instruction.
  //
  return CastInst::CreateZExtOrBitCast (V, Ty, Name, InsertPt);
}

//
// Method: buildBounce()
//
// Description:
//  Replaces the given call site with a call to a bounce function.  The
//  bounce function compares the function pointer to one of the given
//  target functions and calls the function directly if the pointer
//  matches.
//
Function*
Devirtualize::buildBounce (CallSite CS, std::vector<const Function*>& Targets) {
  //
  // Update the statistics on the number of bounce functions added to the
  // module.
  //
  ++FuncAdded;

  //
  // Create a bounce function that has a function signature almost identical
  // to the function being called.  The only difference is that it will have
  // an additional pointer argument at the beginning of its argument list that
  // will be the function to call.
  //
  Value* ptr = CS.getCalledValue();
  const FunctionType* OrigType = 
    cast<FunctionType>(cast<PointerType>(ptr->getType())->getElementType());

  std::vector<const Type *> TP (OrigType->param_begin(), OrigType->param_end());
  TP.insert (TP.begin(), ptr->getType());
  const FunctionType* NewTy = FunctionType::get(OrigType->getReturnType(), TP, false);
  Module * M = CS.getInstruction()->getParent()->getParent()->getParent();
  Function* F = Function::Create (NewTy,
                                  GlobalValue::InternalLinkage,
                                  "devirtbounce",
                                  M);

  //
  // Set the names of the arguments.  Also, record the arguments in a vector
  // for subsequence access.
  //
  F->arg_begin()->setName("funcPtr");
  std::vector<Value*> fargs;
  for(Function::arg_iterator ai = F->arg_begin(), ae = F->arg_end(); ai != ae; ++ai)
    if (ai != F->arg_begin()) {
      fargs.push_back(ai);
      ai->setName("arg");
    }

  //
  // For each function target, create a basic block that will call that
  // function directly.
  //
  std::map<const Function*, BasicBlock*> targets;
  for (unsigned index = 0; index < Targets.size(); ++index) {
    const Function* FL = Targets[index];

    // Create the basic block for doing the direct call
    BasicBlock* BL = BasicBlock::Create (M->getContext(), FL->getName(), F);
    targets[FL] = BL;

    // Create the direct function call
    Value* directCall = CallInst::Create ((Value *)FL,
                                          fargs.begin(),
                                          fargs.end(),
                                          "",
                                          BL);

    // Add the return instruction for the basic block
    if (OrigType->getReturnType()->isVoidTy())
      ReturnInst::Create (M->getContext(), BL);
    else
      ReturnInst::Create (M->getContext(), directCall, BL);
  }

  //
  // Create a set of tests that search for the correct function target
  // and call it directly.  If none of the target functions match,
  // abort (or make the result unreachable).
  //

  //
  // Create the failure basic block.  This basic block should simply be an
  // unreachable instruction.
  //
  BasicBlock* tail = BasicBlock::Create (M->getContext(),
                                         "fail",
                                         F,
                                         &F->getEntryBlock());
  Instruction * InsertPt;
  InsertPt = new UnreachableInst (M->getContext(), tail);

  //
  // Create basic blocks for valid target functions.
  //
  for (unsigned index = 0; index < Targets.size(); ++index) {
    const Function * Target = Targets[index];
    BasicBlock* TB = targets[Target];
    BasicBlock* newB = BasicBlock::Create (M->getContext(),
                                           "test." + Target->getName(),
                                           F,
                                           &F->getEntryBlock());
    CmpInst * setcc = CmpInst::Create (Instruction::ICmp,
                                       CmpInst::ICMP_EQ,
                                       (Value *) Target,
                                       &(*(F->arg_begin())),
                                       "sc",
                                       newB);
    BranchInst::Create (TB, tail, setcc, newB);
    tail = newB;
  }
  return F;
}

//
// Method: makeDirectCall()
//
// Description:
//  Transform the specified call site into a direct call.
//
// Inputs:
//  CS - The call site to transform.
//
// Preconditions:
//  1) This method assumes that CS is an indirect call site.
//  2) This method assumes that a pointer to the CallTarget analysis pass has
//     already been acquired by the class.
//
void
Devirtualize::makeDirectCall (CallSite & CS) {
  //
  // Find the targets of the indirect function call.
  //
  std::vector<const Function*> Targets;
  Targets.insert (Targets.begin(), CTF->begin(CS), CTF->end(CS));

  //
  // Convert the call site if there were any function call targets found.
  //
  if (Targets.size() > 0) {
    //
    // Build a function which will implement a switch statement.  The switch
    // statement will determine which function target to call and call it.
    //
    Function* NF = buildBounce (CS, Targets);

    //
    // Replace the original call with a call to the bounce function.
    //
    if (CallInst* CI = dyn_cast<CallInst>(CS.getInstruction())) {
      std::vector<Value*> Params (CI->op_begin(), CI->op_end());
      CallInst* CN = CallInst::Create (NF,
                                       Params.begin(),
                                       Params.end(),
                                       CI->getName() + ".dv",
                                       CI);
      CI->replaceAllUsesWith(CN);
      CI->eraseFromParent();
    } else if (InvokeInst* CI = dyn_cast<InvokeInst>(CS.getInstruction())) {
      std::vector<Value*> Params (CI->op_begin(), CI->op_end());
      InvokeInst* CN = InvokeInst::Create(NF,
                                          CI->getNormalDest(),
                                          CI->getUnwindDest(),
                                          Params.begin(),
                                          Params.end(),
                                          CI->getName()+".dv",
                                          CI);
      CI->replaceAllUsesWith(CN);
      CI->eraseFromParent();
    }

    //
    // Update the statistics on the number of transformed call sites.
    //
    ++CSConvert;
  }

  return;
}

//
// Method: visitCallSite()
//
// Description:
//  Examine the specified call site.  If it is an indirect call, mark it for
//  transformation into a direct call.
//
void
Devirtualize::visitCallSite (CallSite &CS) {
  //
  // First, determine if this is a direct call.  If so, then just ignore it.
  //
  Value * CalledValue = CS.getCalledValue();
  if (isa<Function>(CalledValue->stripPointerCasts()))
    return;

  //
  // Second, we will only transform those call sites which are complete (i.e.,
  // for which we know all of the call targets).
  //
  if (!(CTF->isComplete(CS)))
    return;

  //
  // This is an indirect call site.  Put it in the worklist of call sites to
  // transforms.
  //
  Worklist.push_back (CS.getInstruction());
  return;
}

//
// Method: runOnModule()
//
// Description:
//  Entry point for this LLVM transform pass.  Look for indirect function calls
//  and turn them into direct function calls.
//
bool
Devirtualize::runOnModule (Module & M) {
  //
  // Get the targets of indirect function calls.
  //
  CTF = &getAnalysis<CallTargetFinder>();

  //
  // Visit all of the call instructions in this function and record those that
  // are indirect function calls.
  //
  visit (M);

  //
  // Now go through and transform all of the indirect calls that we found that
  // need transforming.
  //
  for (unsigned index = 0; index < Worklist.size(); ++index) {
    // Autobots, transform (the call site)!
    CallSite CS (Worklist[index]);
    makeDirectCall (CS);
  }

  //
  // Conservatively assume that we've changed one or more call sites.
  //
  return true;
}

