//===---------- TypeChecksOpt.h - Remove safe runtime type checks ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass removes type checks that are statically proven safe
//
//===----------------------------------------------------------------------===//

#include "assistDS/TypeChecksCmpOpt.h"
#include "llvm/Constants.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Module.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Intrinsics.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/Statistic.h"

#include <set>
#include <vector>

using namespace llvm;

char TypeChecksCmpOpt::ID = 0;

static RegisterPass<TypeChecksCmpOpt> 
TC("typechecks-cmp-opt", "Remove runtime type checks", false, true);

// Pass statistics
STATISTIC(numSafe,  "Number of type checks on loads used in Cmp");

static const Type *VoidTy = 0;
static const Type *Int8Ty = 0;
static const Type *Int32Ty = 0;
static const Type *Int64Ty = 0;
static const PointerType *VoidPtrTy = 0;
static Constant *trackLoadInst;

bool TypeChecksCmpOpt::runOnModule(Module &M) {

  // Create the necessary prototypes
  VoidTy = IntegerType::getVoidTy(M.getContext());
  Int8Ty = IntegerType::getInt8Ty(M.getContext());
  Int32Ty = IntegerType::getInt32Ty(M.getContext());
  Int64Ty = IntegerType::getInt64Ty(M.getContext());
  VoidPtrTy = PointerType::getUnqual(Int8Ty);

  trackLoadInst = M.getOrInsertFunction("trackLoadInst",
                                        VoidTy,
                                        VoidPtrTy,/*ptr*/
                                        Int8Ty,/*type*/
                                        Int64Ty,/*size*/
                                        Int32Ty,/*tag*/
                                        NULL);
  for(Value::use_iterator User = trackLoadInst->use_begin(); User != trackLoadInst->use_end(); ++User) {
    CallInst *CI = dyn_cast<CallInst>(User);
    assert(CI);
    Value *LPtr = CI->getOperand(1)->stripPointerCasts();
    for(Value::use_iterator II = LPtr->use_begin(); II != LPtr->use_end(); ++II) {
      if(LoadInst *LI = dyn_cast<LoadInst>(II)) {
        if(LI->getOperand(0) == LPtr) {
          if(LI->getType()->isPointerTy()) {
            bool allIcmpUse = true;
            for(Value::use_iterator EE = LI->use_begin(); EE != LI->use_end(); ++EE) {
              if(!isa<ICmpInst>(EE)) {
                allIcmpUse = false;
                break;
              }
            }
            if(allIcmpUse) {
              toDelete.insert(CI);
              LI->dump();
            }
          }
        }
      }
    }
  }

  numSafe += toDelete.size();

  std::set<Instruction*>::iterator II = toDelete.begin();
  for(; II != toDelete.end();) {
    Instruction *I = *II++;
    I->eraseFromParent();
  }
  return (numSafe > 0);
}

