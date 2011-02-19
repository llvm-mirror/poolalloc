#define DEBUG_TYPE "simplifygep"

#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Instructions.h"
#include "llvm/Constants.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Debug.h"
#include "llvm/Operator.h"
#include "llvm/Target/TargetData.h"

#include <vector>

using namespace llvm;

namespace {
  static void preprocess(Module& M) {
    for (Module::iterator F = M.begin(); F != M.end(); ++F){
      for (Function::iterator B = F->begin(), FE = F->end(); B != FE; ++B) {      
        for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE; I++) {
          if(!(isa<GetElementPtrInst>(I)))
            continue;
          GetElementPtrInst *GEP = cast<GetElementPtrInst>(I);
          if(BitCastInst *BI = dyn_cast<BitCastInst>(GEP->getOperand(0))) {
            if(Constant *C = dyn_cast<Constant>(BI->getOperand(0))) {
              GEP->setOperand(0, ConstantExpr::getBitCast(C, BI->getType()));
            }
          }
        }
      }
    }
  }
  class SimplifyGEP : public ModulePass {
  private:
    TargetData * TD;
  public:
    static char ID;
    SimplifyGEP() : ModulePass(&ID) {}
    bool runOnModule(Module& M) {
    TD = &getAnalysis<TargetData>();
      preprocess(M);
      //bool changed = false;
      for (Module::iterator F = M.begin(); F != M.end(); ++F){
        for (Function::iterator B = F->begin(), FE = F->end(); B != FE; ++B) {      
          for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE; I++) {
            if(!(isa<GetElementPtrInst>(I)))
              continue;
            GetElementPtrInst *GEP = cast<GetElementPtrInst>(I);
            Value *PtrOp = GEP->getOperand(0);
            Value *StrippedPtr = PtrOp->stripPointerCasts();
            if (StrippedPtr != PtrOp) {
              const PointerType *StrippedPtrTy =cast<PointerType>(StrippedPtr->getType());
              bool HasZeroPointerIndex = false;
              if (ConstantInt *C = dyn_cast<ConstantInt>(GEP->getOperand(1)))
                HasZeroPointerIndex = C->isZero();
              if (HasZeroPointerIndex) {
                const PointerType *CPTy = cast<PointerType>(PtrOp->getType());
                if (const ArrayType *CATy =
                    dyn_cast<ArrayType>(CPTy->getElementType())) {
                  if (CATy->getElementType() == StrippedPtrTy->getElementType()) {
                    SmallVector<Value*, 8> Idx(GEP->idx_begin()+1, GEP->idx_end());
                    GetElementPtrInst *Res =
                      GetElementPtrInst::Create(StrippedPtr, Idx.begin(),
                                                Idx.end(), GEP->getName(), GEP);
                    Res->setIsInBounds(GEP->isInBounds());
                    GEP->replaceAllUsesWith(Res);
                    continue;
                  }
                  
                  if (const ArrayType *XATy =
                      dyn_cast<ArrayType>(StrippedPtrTy->getElementType())){
                    if (CATy->getElementType() == XATy->getElementType()) {
                      GEP->setOperand(0, StrippedPtr);
                      continue;
                    }
                  }
                }   
              } else if (GEP->getNumOperands() == 2) {
                // Transform things like:
                // %t = getelementptr i32* bitcast ([2 x i32]* %str to i32*), i32 %V
                // into:  %t1 = getelementptr [2 x i32]* %str, i32 0, i32 %V; bitcast
                const Type *SrcElTy = StrippedPtrTy->getElementType();
                const Type *ResElTy=cast<PointerType>(PtrOp->getType())->getElementType();
                if (TD && SrcElTy->isArrayTy() &&
                    TD->getTypeAllocSize(cast<ArrayType>(SrcElTy)->getElementType()) ==
                    TD->getTypeAllocSize(ResElTy)) {
                  Value *Idx[2];
                  Idx[0] = Constant::getNullValue(Type::getInt32Ty(GEP->getContext()));
                  Idx[1] = GEP->getOperand(1);
                  Value *NewGEP = GetElementPtrInst::Create(StrippedPtr, Idx,
                                                Idx+2, GEP->getName(), GEP);
                  // V and GEP are both pointer types --> BitCast
                  GEP->replaceAllUsesWith(new BitCastInst(NewGEP, GEP->getType(), GEP->getName(), GEP));
                  continue;
                }
                
                // Transform things like:
                // getelementptr i8* bitcast ([100 x double]* X to i8*), i32 %tmp
                //   (where tmp = 8*tmp2) into:
                // getelementptr [100 x double]* %arr, i32 0, i32 %tmp2; bitcast
                
                if (TD && SrcElTy->isArrayTy() && ResElTy->isIntegerTy(8)) {
                  uint64_t ArrayEltSize =
                    TD->getTypeAllocSize(cast<ArrayType>(SrcElTy)->getElementType());
                  
                  // Check to see if "tmp" is a scale by a multiple of ArrayEltSize.  We
                 // allow either a mul, shift, or constant here.
                  Value *NewIdx = 0;
                  ConstantInt *Scale = 0;
                  if (ArrayEltSize == 1) {
                    NewIdx = GEP->getOperand(1);
                    Scale = ConstantInt::get(cast<IntegerType>(NewIdx->getType()), 1);
                  } else if (ConstantInt *CI = dyn_cast<ConstantInt>(GEP->getOperand(1))) {
                    NewIdx = ConstantInt::get(CI->getType(), 1);
                    Scale = CI;
                  } else if (Instruction *Inst =dyn_cast<Instruction>(GEP->getOperand(1))){
                    if (Inst->getOpcode() == Instruction::Shl &&
                        isa<ConstantInt>(Inst->getOperand(1))) {
                      ConstantInt *ShAmt = cast<ConstantInt>(Inst->getOperand(1));
                      uint32_t ShAmtVal = ShAmt->getLimitedValue(64);
                      Scale = ConstantInt::get(cast<IntegerType>(Inst->getType()),
                                               1ULL << ShAmtVal);
                      NewIdx = Inst->getOperand(0);
                    } else if (Inst->getOpcode() == Instruction::Mul &&
                               isa<ConstantInt>(Inst->getOperand(1))) {
                      Scale = cast<ConstantInt>(Inst->getOperand(1));
                      NewIdx = Inst->getOperand(0);
                    }
                  }
                  
                  // If the index will be to exactly the right offset with the scale taken
                  // out, perform the transformation. Note, we don't know whether Scale is
                  // signed or not. We'll use unsigned version of division/modulo
                  // operation after making sure Scale doesn't have the sign bit set.
                  if (ArrayEltSize && Scale && Scale->getSExtValue() >= 0LL &&
                      Scale->getZExtValue() % ArrayEltSize == 0) {
                    Scale = ConstantInt::get(Scale->getType(),
                                             Scale->getZExtValue() / ArrayEltSize);
                    if (Scale->getZExtValue() != 1) {
                      Constant *C = ConstantExpr::getIntegerCast(Scale, NewIdx->getType(),
                                                                 false /*ZExt*/);
                      NewIdx = BinaryOperator::Create(BinaryOperator::Mul, NewIdx, C, "idxscale");
                    }
                    
                    // Insert the new GEP instruction.
                    Value *Idx[2];
                    Idx[0] = Constant::getNullValue(Type::getInt32Ty(GEP->getContext()));
                    Idx[1] = NewIdx;
                    Value *NewGEP = GetElementPtrInst::Create(StrippedPtr, Idx,
                                                Idx+2, GEP->getName(), GEP);
                    GEP->replaceAllUsesWith(new BitCastInst(NewGEP, GEP->getType(), GEP->getName(), GEP));
                    continue;
                  }
                }
              }
            }
          }
        }
      }

      return true;
    }
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<TargetData>();
    }
  };
}

char SimplifyGEP::ID = 0;
static RegisterPass<SimplifyGEP>
X("simplifygep", "Simplify GEPs");
