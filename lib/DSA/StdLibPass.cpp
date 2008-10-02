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

struct libAction {
  bool ret_read, ret_write, ret_heap;
  bool args_read, args_write, args_heap;
  bool mergeAllArgs;
  bool mergeWithRet;
  bool collapse;
};

const struct {
  const char* name;
  libAction action;
} recFuncs[] = {
  {"calloc",   {false,  true,  true, false, false, false, false, false, false}},
  {"malloc",   {false,  true,  true, false, false, false, false, false, false}},
  {"valloc",   {false,  true,  true, false, false, false, false, false, false}},
  {"memalign", {false,  true,  true, false, false, false, false, false, false}},
  {"strdup",   {false,  true,  true, false, false, false, false, false,  true}},
  {"wcsdup",   {false,  true,  true, false, false, false, false, false,  true}},
  {"free",     {false, false, false, false,  true,  true, false, false, false}},
  {"realloc",  {false,  true,  true, false,  true,  true, false,  true,  true}},
  {"atoi",     {false, false, false,  true, false, false, false, false, false}},
  {"atof",     {false, false, false,  true, false, false, false, false, false}},
  {"atol",     {false, false, false,  true, false, false, false, false, false}},
  {"atoll",    {false, false, false,  true, false, false, false, false, false}},
  {"remove",   {false, false, false,  true, false, false, false, false, false}},
  {"unlink",   {false, false, false,  true, false, false, false, false, false}},
  {"rename",   {false, false, false,  true, false, false, false, false, false}},
  {"memcmp",   {false, false, false,  true, false, false, false, false, false}},
  {"strcmp",   {false, false, false,  true, false, false, false, false, false}},
  {"strncmp",  {false, false, false,  true, false, false, false, false, false}},
  {"execl",    {false, false, false,  true, false, false, false, false, false}},
  {"execlp",   {false, false, false,  true, false, false, false, false, false}},
  {"execle",   {false, false, false,  true, false, false, false, false, false}},
  {"execv",    {false, false, false,  true, false, false, false, false, false}},
  {"execvp",   {false, false, false,  true, false, false, false, false, false}},
  {"chmod",    {false, false, false,  true, false, false, false, false, false}},
  {"puts",     {false, false, false,  true, false, false, false, false, false}},
  {"write",    {false, false, false,  true, false, false, false, false, false}},
  {"open",     {false, false, false,  true, false, false, false, false, false}},
  {"create",   {false, false, false,  true, false, false, false, false, false}},
  {"truncate", {false, false, false,  true, false, false, false, false, false}},
  {"chdir",    {false, false, false,  true, false, false, false, false, false}},
  {"mkdir",    {false, false, false,  true, false, false, false, false, false}},
  {"rmdir",    {false, false, false,  true, false, false, false, false, false}},
  {"strlen",   {false, false, false,  true, false, false, false, false, false}},
  {"read",     {false, false, false, false,  true, false, false, false, false}},
  {"pipe",     {false, false, false, false,  true, false, false, false, false}},
  {"wait",     {false, false, false, false,  true, false, false, false, false}},
  {"time",     {false, false, false, false,  true, false, false, false, false}},
  {"getrusage",{false, false, false, false,  true, false, false, false, false}},
  {"memchr",   { true, false, false,  true, false, false, false,  true,  true}},
  {"memrchr",  { true, false, false,  true, false, false, false,  true,  true}},
  {"rawmemchr",{ true, false, false,  true, false, false, false,  true,  true}},
  {"memmove",  {false,  true, false,  true,  true, false,  true,  true,  true}},
  {"bcopy",    {false, false, false,  true,  true, false,  true, false,  true}},
  {"strcpy",   {false,  true, false,  true,  true, false,  true,  true,  true}},
  {"strncpy",  {false,  true, false,  true,  true, false,  true,  true,  true}},
  {"memccpy",  {false,  true, false,  true,  true, false,  true,  true,  true}},
  {"wcscpy",   {false,  true, false,  true,  true, false,  true,  true,  true}},
  {"wcsncpy",  {false,  true, false,  true,  true, false,  true,  true,  true}},
  {"wmemccpy", {false,  true, false,  true,  true, false,  true,  true,  true}},
};

bool StdLibDataStructures::runOnModule(Module &M) {
  LocalDataStructures &LocalDSA = getAnalysis<LocalDataStructures>();
  setGraphSource(&LocalDSA);
  setTargetData(LocalDSA.getTargetData());
  setGraphClone(false);
  GlobalECs = LocalDSA.getGlobalECs();

  GlobalsGraph = new DSGraph(LocalDSA.getGlobalsGraph(), GlobalECs);
  GlobalsGraph->setPrintAuxCalls();

  //Clone Module
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) 
    if (!I->isDeclaration())
      getOrCreateGraph(&*I);

  for (int x = 0; recFuncs[x].name; ++x)
    if (Function* F = M.getFunction(recFuncs[x].name))
      if (F->isDeclaration())
        for (Value::use_iterator ii = F->use_begin(), ee = F->use_end();
             ii != ee; ++ii)
          if (CallInst* CI = dyn_cast<CallInst>(ii))
            if (CI->getOperand(0) == F) {
              DSGraph& Graph = getDSGraph(*CI->getParent()->getParent());
              if (recFuncs[x].action.ret_read)
                Graph.getNodeForValue(CI).getNode()->setReadMarker();
              if (recFuncs[x].action.ret_write)
                Graph.getNodeForValue(CI).getNode()->setModifiedMarker();
              if (recFuncs[x].action.ret_heap)
                Graph.getNodeForValue(CI).getNode()->setHeapMarker();

              if (recFuncs[x].action.args_read)
                for (unsigned y = 1; y < CI->getNumOperands(); ++y)
                  if (isa<PointerType>(CI->getOperand(y)->getType()))
                    Graph.getNodeForValue(CI->getOperand(y)).getNode()->setReadMarker();
              if (recFuncs[x].action.args_write)
                for (unsigned y = 1; y < CI->getNumOperands(); ++y)
                  if (isa<PointerType>(CI->getOperand(y)->getType()))
                    if (DSNode * Node=Graph.getNodeForValue(CI->getOperand(y)).getNode())
                      Node->setModifiedMarker();
              if (recFuncs[x].action.args_heap)
                for (unsigned y = 1; y < CI->getNumOperands(); ++y)
                  if (isa<PointerType>(CI->getOperand(y)->getType()))
                    Graph.getNodeForValue(CI->getOperand(y)).getNode()->setHeapMarker();

              std::vector<DSNodeHandle> toMerge;
              if (recFuncs[x].action.mergeWithRet)
                toMerge.push_back(Graph.getNodeForValue(CI));
              if (recFuncs[x].action.mergeAllArgs || recFuncs[x].action.mergeWithRet)
                for (unsigned y = 1; y < CI->getNumOperands(); ++y)
                  if (isa<PointerType>(CI->getOperand(y)->getType()))
                    toMerge.push_back(Graph.getNodeForValue(CI->getOperand(y)));
              for (unsigned y = 1; y < toMerge.size(); ++y)
                toMerge[0].mergeWith(toMerge[y]);

              if (recFuncs[x].action.collapse) {
                Graph.getNodeForValue(CI).getNode()->foldNodeCompletely();
                for (unsigned y = 1; y < CI->getNumOperands(); ++y)
                  if (isa<PointerType>(CI->getOperand(y)->getType()))
                    Graph.getNodeForValue(CI->getOperand(y)).getNode()->foldNodeCompletely();
              }

              //delete the call
              DOUT << "Removing " << F->getName() << " from " << CI->getParent()->getParent()->getName() << "\n";
              Graph.removeFunctionCalls(*F);
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

