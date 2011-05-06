//===-------- ArgCast.cpp - Cast Arguments to Calls -----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Convert 
// call(bitcast (.., T1 arg, ...)F to(..., T2 arg, ...))(..., T2 val, ...)
// to
// val1 = bitcast T2 val to T1
// call F (..., T1 val1, ...)
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "argcast"

#include "assistDS/ArgCast.h"
#include "llvm/Attributes.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Debug.h"

#include <set>
#include <map>
#include <vector>

using namespace llvm;

// Pass statistics
STATISTIC(numChanged,   "Number of Args bitcasted");

//
// Method: runOnModule()
//
// Description:
//  Entry point for this LLVM pass.
//  Search for all call sites to casted functions.
//  Check if they only differ in an argument type
//  Cast the argument, and call the original function
//
// Inputs:
//  M - A reference to the LLVM module to transform
//
// Outputs:
//  M - The transformed LLVM module.
//
// Return value:
//  true  - The module was modified.
//  false - The module was not modified.
//
bool ArgCast::runOnModule(Module& M) {

  std::vector<CallInst*> worklist;
  for (Module::iterator I = M.begin(); I != M.end(); ++I)
    if (!I->isDeclaration() && !I->mayBeOverridden())
      // Find all uses of this function
      for(Value::use_iterator ui = I->use_begin(), ue = I->use_end(); ui != ue; ++ui)
        // check if is ever casted to a different function type
        if (Constant *C = dyn_cast<Constant>(ui)) 
          if (ConstantExpr *CE = dyn_cast<ConstantExpr>(C)) 
            if (CE->getOpcode() == Instruction::BitCast) 
              if(CE->getOperand(0) == I) 
                if(const FunctionType *FTy  = dyn_cast<FunctionType>
                   ((cast<PointerType>(CE->getType()))->getElementType())) {
                  //casting to a varargs funtion
                  if(FTy->isVarArg())
                    for(Value::use_iterator uii = CE->use_begin(),
                        uee = CE->use_end(); uii != uee; ++uii) {
                      // Find all uses of the casted value, and check if it is 
                      // used in a Call Instruction
                      if (CallInst* CI = dyn_cast<CallInst>(uii)) {
                        // Check that it is the called value, and not an argument
                        if(CI->getCalledValue() != CE) 
                          continue;
                        // Check that the number of arguments passed, and expected
                        // by the function are the same.
                        if(CI->getNumOperands() != I->arg_size() + 1)
                          continue;
                        // Check that the return type of the function matches that
                        // expected by the call inst(ensures that the reason for the
                        // cast is not the return type).
                        if(CI->getType() != I->getReturnType())
                          continue;

                        // If so, add to worklist
                        worklist.push_back(CI);
                      }
                    }
                }

  // Proces the worklist of potential call sites to transform
  while(!worklist.empty()) {
    CallInst *CI = worklist.back();
    worklist.pop_back();
    // Get the called Function
    Function *F = cast<Function>(CI->getCalledValue()->stripPointerCasts());
    const FunctionType *FTy = F->getFunctionType();

    SmallVector<Value*, 8> Args;
    unsigned i =0;
    for(i =0; i< FTy->getNumParams(); ++i) {
      const Type *ArgType = CI->getOperand(i+1)->getType();
      const Type *FormalType = FTy->getParamType(i);
      // If the types for this argument match, just add it to the
      // parameter list. No cast needs to be inserted.
      if(ArgType == FormalType) {
        Args.push_back(CI->getOperand(i+1));
      }
      else if(ArgType->isPointerTy() && FormalType->isPointerTy()) {
        CastInst *CastI = CastInst::CreatePointerCast(CI->getOperand(i+1), 
                                                      FormalType, "", CI);
        Args.push_back(CastI);
      } else if (ArgType->isIntegerTy() && FormalType->isIntegerTy()) {
        unsigned SrcBits = ArgType->getScalarSizeInBits();
        unsigned DstBits = FormalType->getScalarSizeInBits();
        if(SrcBits > DstBits) {
          CastInst *CastI = CastInst::CreateIntegerCast(CI->getOperand(i+1), 
                                                        FormalType, true, "", CI);
          Args.push_back(CastI);
        } else {
          if(F->paramHasAttr(i+1, Attribute::SExt)) {
            CastInst *CastI = CastInst::CreateIntegerCast(CI->getOperand(i+1), 
                                                          FormalType, true, "", CI);
            Args.push_back(CastI);
          } else if(F->paramHasAttr(i+1, Attribute::ZExt)) {
            CastInst *CastI = CastInst::CreateIntegerCast(CI->getOperand(i+1), 
                                                          FormalType, false, "", CI);
            Args.push_back(CastI);
          } else {
            // Use ZExt in default case.
            // Derived from InstCombine. Also, the only reason this should happen
            // is mismatched prototypes.
            // Seen in case of integer constants which get interpreted as i32, 
            // even if being used as i64.
            // TODO: is this correct?
            CastInst *CastI = CastInst::CreateIntegerCast(CI->getOperand(i+1), 
                                                          FormalType, false, "", CI);
            Args.push_back(CastI);
          } 
        } 
      } else {
        DEBUG(ArgType->dump());
        DEBUG(FormalType->dump());
        break;
      }
    }

    // If we found an argument we could not cast, try the next instruction
    if(i != FTy->getNumParams())
      continue;

    // else replace the call instruction
    CallInst *CINew = CallInst::Create(F, Args.begin(), Args.end(), "", CI);
    CINew->setCallingConv(CI->getCallingConv());
    CINew->setAttributes(CI->getAttributes());
    CI->replaceAllUsesWith(CINew);
    CI->eraseFromParent();
    numChanged++;
  }
  return true;
}

// Pass ID variable
char ArgCast::ID = 0;

// Register the pass
static RegisterPass<ArgCast>
X("arg-cast", "Cast Arguments");
