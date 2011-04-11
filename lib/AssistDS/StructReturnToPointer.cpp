//===-------- StructReturnToPointer.cpp ------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "struct-ret"

#include "llvm/Instructions.h"
#include "llvm/Attributes.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Debug.h"

#include <set>
#include <map>
#include <vector>

using namespace llvm;


namespace {

  class StructArg : public ModulePass {
  public:
    static char ID;
    StructArg() : ModulePass(&ID) {}

    //
    // Method: runOnModule()
    //
    // Description:
    //  Entry point for this LLVM pass.
    //  If a function returns a struct, make it return
    //  a pointer to the struct.
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
    bool runOnModule(Module& M) {

      std::vector<Function*> worklistR;
      for (Module::iterator I = M.begin(); I != M.end(); ++I)
        if (!I->isDeclaration() && !I->mayBeOverridden()) {
          if(I->hasAddressTaken())
            continue;
          if(I->getReturnType()->isStructTy()) {
            worklistR.push_back(I);
          }
        }

      while(!worklistR.empty()) {
        Function *F = worklistR.back();
        worklistR.pop_back();
        const Type *NewReturnType = F->getReturnType()->getPointerTo();

        // Construct the new Type
        std::vector<const Type*>TP;
        for (Function::arg_iterator ii = F->arg_begin(), ee = F->arg_end();
             ii != ee; ++ii) {
          TP.push_back(ii->getType());
        }

        const FunctionType *NFTy = FunctionType::get(NewReturnType, TP, false);

        // Create the new function body and insert it into the module.
        Function *NF = Function::Create(NFTy, F->getLinkage(), F->getName(), &M);
        NF->copyAttributesFrom(F);
        Function::arg_iterator NI = NF->arg_begin();
        for (Function::arg_iterator II = F->arg_begin(); II != F->arg_end(); ++II, ++NI) {
          II->replaceAllUsesWith(NI);
          NI->setName(II->getName());
        }
        NF->getBasicBlockList().splice(NF->begin(), F->getBasicBlockList());
        Instruction *InsertPoint;
        for (BasicBlock::iterator insrt = NF->front().begin(); isa<AllocaInst>(InsertPoint = insrt); ++insrt) {;}

        Function* MallocF = M.getFunction ("malloc");
        const Type* IntPtrT = Type::getInt64Ty(M.getContext());

        const Type *BPTy = Type::getInt8PtrTy(M.getContext());
        Value *MallocFunc = MallocF;
        Constant *Size;
        if (!MallocFunc) {
          // prototype malloc as "void *malloc(size_t)"
          Size = ConstantInt::get (IntPtrT, 123, true);
          MallocFunc = M.getOrInsertFunction("malloc", BPTy, IntPtrT, NULL);
        } else {
          Size = ConstantInt::get (MallocF->arg_begin()->getType(), 123, true);
        }
        CallInst *MCall = CallInst::Create(MallocFunc,Size,  "ret_ptr", InsertPoint);
        Instruction *Result = new BitCastInst(MCall, NewReturnType, "", InsertPoint);
        for (Function::iterator B = NF->begin(), FE = NF->end(); B != FE; ++B) {      
          for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE;) {
            ReturnInst * RI = dyn_cast<ReturnInst>(I++);
            if(!RI)
              continue;
            new StoreInst(RI->getOperand(0), Result, RI);
            ReturnInst::Create(M.getContext(), Result, RI);
            RI->eraseFromParent();
          }
        }

        for(Value::use_iterator ui = F->use_begin(), ue = F->use_end();
            ui != ue; ) {
          CallInst *CI = dyn_cast<CallInst>(ui++);
          SmallVector<Value*, 8> Args;
          for(unsigned j =1;j<CI->getNumOperands();j++) {
            Args.push_back(CI->getOperand(j));
          }
          CallInst *CINew = CallInst::Create(NF, Args.begin(), Args.end(), "", CI);
          LoadInst *LI = new LoadInst(CINew, "", CI);
          CI->replaceAllUsesWith(LI);
          CI->eraseFromParent();
        }
        F->eraseFromParent();
      }
      return true;
    }
  };
}

// Pass ID variable
char StructArg::ID = 0;

// Register the pass
static RegisterPass<StructArg>
X("struct-ret", "Find struct arguments");
