//===-- MergeGEP.cpp - Merge GEPs for indexing in arrays ------------ ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// 
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "mergegep"

#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Instructions.h"
#include "llvm/Constants.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Debug.h"
#include <vector>

using namespace llvm;


namespace {
  class MergeGEP : public ModulePass {
  public:
    static char ID;
    MergeGEP() : ModulePass(&ID) {}
    bool runOnModule(Module& M) {
      bool changed = false;
      bool found;
      do {
      found = false;
      for (Module::iterator F = M.begin(); F != M.end(); ++F){
        for (Function::iterator B = F->begin(), FE = F->end(); B != FE; ++B) {      
          for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE; I++) {
            if(!(isa<GetElementPtrInst>(I)))
              continue;
            GetElementPtrInst *GEP = cast<GetElementPtrInst>(I);
            if(!isa<ArrayType>(GEP->getType()->getElementType()))
              continue;
                  std::vector<GetElementPtrInst*> worklist;
                  for (Value::use_iterator UI = GEP->use_begin(),
                                     UE = GEP->use_end(); UI != UE; ++UI){
                    if(!isa<GetElementPtrInst>(UI))
                      break;
                    GetElementPtrInst *GEPUse = cast<GetElementPtrInst>(UI);
                    worklist.push_back(GEPUse);
                  }
                  while(!worklist.empty()){
                    GetElementPtrInst *GEPUse = worklist.back();
                    worklist.pop_back();
                    SmallVector<Value*, 8> Indices;
                    Indices.append(GEP->op_begin()+1, GEP->op_end());
                    Indices.append(GEPUse->idx_begin()+1, GEPUse->idx_end());
                    GetElementPtrInst *GEPNew = GetElementPtrInst::Create(GEP->getOperand(0),
                                                                          Indices.begin(),
                                                                          Indices.end(),
                                                                          GEPUse->getName()+ "moda", 
                                                                          GEPUse);
                    GEPUse->replaceAllUsesWith(GEPNew);
                    GEPUse->eraseFromParent();        
                    found = true;
                    changed = true;                    
                  }
          }
        }
      std::vector<GetElementPtrInst*> worklist;
      for (Function::iterator B = F->begin(), FE = F->end(); B != FE; ++B) {      
        for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE; I++) {
          if(!(isa<GetElementPtrInst>(I)))
            continue;
          GetElementPtrInst *GEP1 = cast<GetElementPtrInst>(I);
          if(!isa<ArrayType>(GEP1->getType()->getElementType()))
              continue;
          if (Constant *C = dyn_cast<Constant>(GEP1->getOperand(0))) {
            if (ConstantExpr *CE = dyn_cast<ConstantExpr>(C)) {
              if (CE->getOpcode() == Instruction::GetElementPtr) {
                  worklist.push_back(GEP1);
              }
            }
          }
        }
      }
      while(!worklist.empty()) {
        GetElementPtrInst *GEP1 = worklist.back();
        worklist.pop_back();
        Constant *C = cast<Constant>(GEP1->getOperand(0));
        ConstantExpr *CE = cast<ConstantExpr>(C);
        SmallVector<Value*, 8> Indices;
        Indices.append(CE->op_begin()+1, CE->op_end());
        Indices.append(GEP1->idx_begin()+1, GEP1->idx_end());
        GetElementPtrInst *GEPNew = GetElementPtrInst::Create(CE->getOperand(0),
                                                              Indices.begin(),
                                                              Indices.end(),
                                                              GEP1->getName()+ "modb", 
                                                              GEP1);
        GEP1->replaceAllUsesWith(GEPNew);
        GEP1->eraseFromParent();
        changed = true;                    
        found = true;
      }
      }
      }while(found);
      return changed;
    }
  };
}

char MergeGEP::ID = 0;
static RegisterPass<MergeGEP>
X("mergegep", "Merge GEPs for arrays in structs");
