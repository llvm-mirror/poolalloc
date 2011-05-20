//===-- LoadArgs.cpp - Promote args if they came from loads ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Identify calls, that are passed arguments that are LoadInsts.
// Pass the original pointer instead. Helps improve some
// context sensitivity.
// 
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "ld-args"

#include "assistDS/LoadArgs.h"
#include "llvm/Constants.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Debug.h"
#include "llvm/Use.h"
#include <vector>
#include <map>

// Pass statistics
STATISTIC(numSimplified, "Number of Calls Modified");

using namespace llvm;

//
// Method: runOnModule()
//
// Description:
//  Entry point for this LLVM pass.
//  Clone functions that take LoadInsts as arguments
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
bool LoadArgs::runOnModule(Module& M) {
  bool changed;
  do { 
    changed = false;
    for (Module::iterator F = M.begin(); F != M.end(); ++F){
      for (Function::iterator B = F->begin(), FE = F->end(); B != FE; ++B) {
        for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE;) {
          CallInst *CI = dyn_cast<CallInst>(I++);
          if(!CI)
            continue;

          if(CI->hasByValArgument())
            continue;
          // if the CallInst calls a function, that is externally defined,
          // or might be changed, ignore this call site.
          Function *F = CI->getCalledFunction();

          if (!F || (F->isDeclaration() || F->mayBeOverridden())) 
            continue;
          if(F->hasStructRetAttr())
            continue;
          if(F->isVarArg())
            continue;

          // find the argument we must replace
          Function::arg_iterator ai = F->arg_begin(), ae = F->arg_end();
          unsigned argNum = 1;
          for(; argNum < CI->getNumOperands();argNum++, ++ai) {
            // do not care about dead arguments
            if(ai->use_empty())
              continue;
            if(F->paramHasAttr(argNum, Attribute::SExt) ||
               F->paramHasAttr(argNum, Attribute::ZExt)) 
              continue;
            if (isa<LoadInst>(CI->getOperand(argNum)))
              break;
          }

          // if no argument was a GEP operator to be changed 
          if(ai == ae)
            continue;

          LoadInst *LI = dyn_cast<LoadInst>(CI->getOperand(argNum));
          if(LI->getParent() != CI->getParent())
            continue;
          // Also check that there is no store after the load.
          // TODO: Check if the load/store do not alias.
          BasicBlock::iterator bii = LI->getParent()->begin();
          Instruction *BII = bii;
          while(BII != LI) {
            ++bii;
            BII = bii;
          }
          while(BII != CI) {
            if(isa<StoreInst>(BII))
              break;
            ++bii;
            BII = bii;
          }
          if(isa<StoreInst>(bii)){
            continue;
          }

          // Construct the new Type
          // Appends the struct Type at the beginning
          std::vector<const Type*>TP;
          TP.push_back(LI->getOperand(0)->getType());
          for(unsigned c = 1; c < CI->getNumOperands();c++) {
            TP.push_back(CI->getOperand(c)->getType());
          }

          //return type is same as that of original instruction
          const FunctionType *NewFTy = FunctionType::get(CI->getType(), TP, false);
          numSimplified++;
          if(numSimplified > 400)
            return true;

          Function *NewF = Function::Create(NewFTy,
                                            GlobalValue::InternalLinkage,
                                            F->getNameStr() + ".TEST",
                                            &M);

          Function::arg_iterator NI = NewF->arg_begin();
          NI->setName("LDarg");
          ++NI;

          DenseMap<const Value*, Value*> ValueMap;

          for (Function::arg_iterator II = F->arg_begin(); NI != NewF->arg_end(); ++II, ++NI) {
            ValueMap[II] = NI;
            NI->setName(II->getName());
            NI->addAttr(F->getAttributes().getParamAttributes(II->getArgNo() + 1));
          }
          // Perform the cloning.
          SmallVector<ReturnInst*,100> Returns;
          CloneFunctionInto(NewF, F, ValueMap, Returns);
          std::vector<Value*> fargs;
          for(Function::arg_iterator ai = NewF->arg_begin(), 
              ae= NewF->arg_end(); ai != ae; ++ai) {
            fargs.push_back(ai);
          }

          NewF->setAttributes(NewF->getAttributes().addAttr(
              0, F->getAttributes().getRetAttributes()));
          NewF->setAttributes(NewF->getAttributes().addAttr(
              ~0, F->getAttributes().getFnAttributes()));
          //Get the point to insert the GEP instr.
          SmallVector<Value*, 8> Ops(CI->op_begin()+1, CI->op_end());
          Instruction *InsertPoint;
          for (BasicBlock::iterator insrt = NewF->front().begin(); isa<AllocaInst>(InsertPoint = insrt); ++insrt) {;}

          NI = NewF->arg_begin();
          LoadInst *LI_new = new LoadInst(cast<Value>(NI), "", InsertPoint);
          fargs.at(argNum)->replaceAllUsesWith(LI_new);

          SmallVector<AttributeWithIndex, 8> AttributesVec;

          // Get the initial attributes of the call
          AttrListPtr CallPAL = CI->getAttributes();
          Attributes RAttrs = CallPAL.getRetAttributes();
          Attributes FnAttrs = CallPAL.getFnAttributes();
          if (RAttrs)
            AttributesVec.push_back(AttributeWithIndex::get(0, RAttrs));
          
          SmallVector<Value*, 8> Args;
          Args.push_back(LI->getOperand(0));
          for(unsigned j =1;j<CI->getNumOperands();j++) {
            Args.push_back(CI->getOperand(j));
            // position in the AttributesVec
            if (Attributes Attrs = CallPAL.getParamAttributes(j))
              AttributesVec.push_back(AttributeWithIndex::get(Args.size(), Attrs));
          }
          // Create the new attributes vec.
          if (FnAttrs != Attribute::None)
            AttributesVec.push_back(AttributeWithIndex::get(~0, FnAttrs));

          AttrListPtr NewCallPAL = AttrListPtr::get(AttributesVec.begin(),
                                                    AttributesVec.end());
          CallInst *CallI = CallInst::Create(NewF,Args.begin(), Args.end(),"", CI);
          CallI->setCallingConv(CI->getCallingConv());
          CallI->setAttributes(NewCallPAL);
          CI->replaceAllUsesWith(CallI);
          CI->eraseFromParent();
          changed = true;
        }
      }
    }
  } while(changed);
  return true;
}

char LoadArgs::ID = 0;
static RegisterPass<LoadArgs>
X("ld-args", "Find Load Inst passed as args");
