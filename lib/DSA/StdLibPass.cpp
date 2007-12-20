//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Recognize common standard c library functions and generate graphs for them
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/Statistic.h"
#include "dsa/DataStructure.h"
#include "dsa/DSGraph.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Intrinsics.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Timer.h"
#include <iostream>
#include "llvm/Module.h"

using namespace llvm;

static RegisterPass<StdLibDataStructures>
X("dsa-stdlib", "Standard Library Local Data Structure Analysis");

char StdLibDataStructures::ID;

bool StdLibDataStructures::runOnModule(Module &M) {
  LocalDataStructures &LocalDSA = getAnalysis<LocalDataStructures>();
  setGraphSource(&LocalDSA);
  setTargetData(LocalDSA.getTargetData());
  setGraphClone(false);
  GlobalECs = LocalDSA.getGlobalECs();

  GlobalsGraph = new DSGraph(LocalDSA.getGlobalsGraph(), GlobalECs);
  GlobalsGraph->setPrintAuxCalls();

  // Calculate all of the graphs...
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    DSGraph &Graph = getOrCreateGraph(&*I);
    //If this is an true external, check it out
    if (I->isDeclaration() && !I->isIntrinsic()) {
      const std::string& Name = I->getName();
      if (Name == "calloc" ||
          Name == "malloc" ||
          Name == "valloc" ||
          Name == "memalign") {
        Graph.getReturnNodeFor(*I).getNode()->clearNodeFlags()
          ->setHeapMarker()->setModifiedMarker();
      } else if (Name == "realloc") {
        Graph.getReturnNodeFor(*I).getNode()->clearNodeFlags()
          ->setHeapMarker()->setModifiedMarker();
        Graph.getNodeForValue(I->arg_begin()).getNode()->clearNodeFlags()
          ->mergeWith(Graph.getReturnNodeFor(*I), 0);
      } else if (Name == "free") {
        Graph.getNodeForValue(&*I->arg_begin()).getNode()->clearNodeFlags()
          ->setHeapMarker()->setModifiedMarker();
      } else if (Name == "atoi"     || Name == "atof"    ||
                 Name == "atol"     || Name == "atoll"   ||
                 Name == "remove"   || Name == "unlink"  ||
                 Name == "rename"   || Name == "memcmp"  ||
                 Name == "strcmp"   || Name == "strncmp" ||
                 Name == "execl"    || Name == "execlp"  ||
                 Name == "execle"   || Name == "execv"   ||
                 Name == "execvp"   || Name == "chmod"   ||
                 Name == "puts"     || Name == "write"   ||
                 Name == "open"     || Name == "create"  ||
                 Name == "truncate" || Name == "chdir"   ||
                 Name == "mkdir"    || Name == "rmdir"   ||
                 Name == "strlen") {
        for (Function::arg_iterator AI = I->arg_begin(), E = I->arg_end();
             AI != E; ++AI) {
          if (isa<PointerType>(AI->getType()))
            Graph.getNodeForValue(&*AI).getNode()->clearNodeFlags()
              ->setReadMarker();
        }
      } else if (Name == "read" || Name == "pipe" ||
                 Name == "wait" || Name == "time" ||
                 Name == "getrusage") {
        for (Function::arg_iterator AI = I->arg_begin(), E = I->arg_end();
             AI != E; ++AI) {
          if (isa<PointerType>(AI->getType()))
            Graph.getNodeForValue(&*AI).getNode()->clearNodeFlags()
              ->setModifiedMarker();
        }
      } else if (Name == "memchr" || Name == "memrchr") {
        DSNodeHandle RetNH = Graph.getReturnNodeFor(*I);
        DSNodeHandle Result = Graph.getNodeForValue(&*I->arg_begin());
        RetNH.mergeWith(Result);
        RetNH.getNode()->clearNodeFlags()->setReadMarker();
      } else if (Name == "memmove") {
        // Merge the first & second arguments, and mark the memory read and
        // modified.
        DSNodeHandle& RetNH = Graph.getNodeForValue(&*I->arg_begin());
        RetNH.mergeWith(Graph.getNodeForValue(&*(++(I->arg_begin()))));
        RetNH.getNode()->clearNodeFlags()->setModifiedMarker()->setReadMarker();
      } else if (Name == "stat" || Name == "fstat" || Name == "lstat") {
        // These functions read their first operand if its a pointer.
        Function::arg_iterator AI = I->arg_begin();
        if (isa<PointerType>(AI->getType()))
          Graph.getNodeForValue(&*AI).getNode()
            ->clearNodeFlags()->setReadMarker();
        // Then they write into the stat buffer.
        DSNodeHandle StatBuf = Graph.getNodeForValue(&*++AI);
        DSNode *N = StatBuf.getNode();
        N->setModifiedMarker();
        const Type *StatTy = I->getFunctionType()->getParamType(1);
        if (const PointerType *PTy = dyn_cast<PointerType>(StatTy))
          N->mergeTypeInfo(PTy->getElementType(), StatBuf.getOffset());
      } else if (Name == "strtod" || Name == "strtof" || Name == "strtold") {
        // These functions read the first pointer
        DSNodeHandle& Str = Graph.getNodeForValue(&*I->arg_begin());
        Str.getNode()->clearNodeFlags()->setReadMarker();
        // If the second parameter is passed, it will point to the first
        // argument node.
        DSNodeHandle& EndNH = Graph.getNodeForValue(&*(++(I->arg_begin())));
        EndNH.getNode()->clearNodeFlags()->setModifiedMarker();
        EndNH.getNode()->mergeTypeInfo(PointerType::getUnqual(Type::Int8Ty),
                                       EndNH.getOffset(), false);
        DSNodeHandle &Link = EndNH.getLink(0);
        Link.mergeWith(Str);
      } else {
        //ignore pointer free functions
        bool hasPtr = isa<PointerType>(I->getReturnType());
        for (Function::const_arg_iterator AI = I->arg_begin(), AE = I->arg_end();
             AI != AE && !hasPtr; ++AI)
          if (isa<PointerType>(AI->getType()))
            hasPtr = true;
        if (hasPtr)
          std::cerr << "Unhandled External: " << Name << "\n";
      }
    }
  }

  return false;
}

// releaseMemory - If the pass pipeline is done with this pass, we can release
// our memory... here...
//
void StdLibDataStructures::releaseMemory() {
  for (hash_map<Function*, DSGraph*>::iterator I = DSInfo.begin(),
         E = DSInfo.end(); I != E; ++I) {
    I->second->getReturnNodes().erase(I->first);
    if (I->second->getReturnNodes().empty())
      delete I->second;
  }

  // Empty map so next time memory is released, data structures are not
  // re-deleted.
  DSInfo.clear();
  delete GlobalsGraph;
  GlobalsGraph = 0;
}

