//===-- VarArgsFunc.cpp - Simplify calls to bitcasted const funcs --------===//
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
#define DEBUG_TYPE "varargfunc"

#include "assistDS/VarArgsFunc.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Debug.h"

#include <set>
#include <map>
#include <vector>

using namespace llvm;

// Pass statistics
STATISTIC(numSimplified, "Number of Calls Simplified");

//
// Method: runOnModule()
// Description:
//  Entry point for this LLVM pass. Search for functions that are
//  unnecessarily casted to varargs type, in a CallInst.
//  Replace with direct calls to the function
//
// Inputs:
// M - A reference to the LLVM module to transform.
//
// Outputs:
// M - The transformed LLVM module.
//
// Return value:
//  true  - The module was modified.
//  false - The module was not modified.
//
bool VarArgsFunc::runOnModule(Module& M) {
  std::vector<CallInst*> worklist;

  for (Module::iterator I = M.begin(); I != M.end(); ++I) {
    // Go through all the functions
    if (I->mayBeOverridden()) 
      continue;
    //Uses of Function I
    for(Value::use_iterator ui = I->use_begin(), ue = I->use_end();
        ui != ue; ++ui) 
      //Find all casted uses of the function
      if (Constant *C = dyn_cast<Constant>(ui)) 
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(C)) 
          if (CE->getOpcode() == Instruction::BitCast) 
            if(CE->getOperand(0) == I) 
              if(const FunctionType *FTy  = dyn_cast<FunctionType>
                 ((cast<PointerType>(CE->getType()))->getElementType())) 
                //casting to a varargs funtion
                if(FTy->isVarArg()) { 
                  // Check if bitcasted Value is used in a callInst
                  for(Value::use_iterator uii = CE->use_begin(),
                      uee = CE->use_end(); uii != uee; ++uii) 
                    if (CallInst* CI = dyn_cast<CallInst>(uii)) 
                      if(CI->getCalledValue() == CE) { 
                        // add to a worklist to process
                        worklist.push_back(CI);
                      }
                }
  }

  // process the worklist
  while(!worklist.empty()) {
    CallInst *CI = worklist.back();
    worklist.pop_back();
    Function *F = cast<Function>(CI->getCalledValue()->stripPointerCasts());
    // Only continue, if we are passing the exact number of arguments
    if(F->arg_size() != (CI->getNumOperands()-1))
      continue;
    // Only continue if we are getting the same return type value
    // Or we can discard the returned value.
    if(F->getReturnType() != CI->getType()) {
      if(!CI->use_empty())
        continue;
    }
    // Check if the parameters passed match the expected types of the 
    // formal arguments
    bool change = true;
    unsigned arg_count = 1;
    for (Function::arg_iterator ii = F->arg_begin(), ee = F->arg_end();ii != ee; ++ii,arg_count++) {
      if(ii->getType() != CI->getOperand(arg_count)->getType()) {
        change = false;
        break;
      }
    }

    if(change) {
      // if we want to ignore the returned value, create a new CallInst
      SmallVector<Value*, 8> Args;
      for(unsigned j =1;j<CI->getNumOperands();j++) {
        Args.push_back(CI->getOperand(j));
      }
      CallInst *CINew = CallInst::Create(F, Args.begin(), Args.end(), "", CI);
      if(F->getReturnType() == CI->getType()){ // else means no uses
        CI->replaceAllUsesWith(CINew);
      }
      DEBUG(errs() << "VA:");
      DEBUG(errs() << "ERASE:");
      DEBUG(CI->dump());
      DEBUG(errs() << "VA:");
      DEBUG(errs() << "ADDED:");
      DEBUG(CINew->dump());
      CI->eraseFromParent();
      numSimplified++;
    }
  }
  return (numSimplified > 0 );
}

// Pass ID variable
char VarArgsFunc::ID = 0;

// Register the Pass
static RegisterPass<VarArgsFunc>
X("varargsfunc", "Optimize non-varargs to varargs function casts");
