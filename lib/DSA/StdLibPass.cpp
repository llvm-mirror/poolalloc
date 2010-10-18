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
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Timer.h"
#include <iostream>
#include "llvm/Module.h"

using namespace llvm;

static RegisterPass<StdLibDataStructures>
X("dsa-stdlib", "Standard Library Local Data Structure Analysis");

char StdLibDataStructures::ID;

#define numOps 10

//
// Structure: libAction
//
// Description:
//  Describe how the DSGraph of a function should be built.  Note that for the
//  boolean arrays of arity numOps, the first element is a flag describing the
//  return value, and the remaining elements are flags describing the
//  function's arguments.
//
struct libAction {
  // The return value/arguments that should be marked read.
  bool read[numOps];

  // The return value/arguments that should be marked modified.
  bool write[numOps];

  // The return value/arguments that should be marked as heap.
  bool heap[numOps];

  // Flags whether all arguments should be merged together.
  bool mergeAllArgs;

  // Flags whether the return value should be merged with all arguments.
  bool mergeWithRet;

  // Flags whether the return value and arguments should be folded.
  bool collapse;
};

#define NRET_NARGS  {0,0,0,0,0,0,0,0,0,0}
#define YRET_NARGS  {1,0,0,0,0,0,0,0,0,0}
#define NRET_YARGS  {0,1,1,1,1,1,1,1,1,1}
#define YRET_YARGS  {1,1,1,1,1,1,1,1,1,1}
#define NRET_NYARGS {0,0,1,1,1,1,1,1,1,1}
#define YRET_NYARGS {1,0,1,1,1,1,1,1,1,1}
#define NRET_YNARGS {0,1,0,0,0,0,0,0,0,0}
#define YRET_YNARGS {1,1,0,0,0,0,0,0,0,0}


const struct {
  const char* name;
  libAction action;
} recFuncs[] = {
  {"stat",       {NRET_YNARGS, NRET_NYARGS, NRET_NARGS, false, false, false}},
  {"fstat",      {NRET_YNARGS, NRET_NYARGS, NRET_NARGS, false, false, false}},
  {"lstat",      {NRET_YNARGS, NRET_NYARGS, NRET_NARGS, false, false, false}},

  // printf not strictly true, %n could cause a write
  {"printf",     {NRET_YARGS,  NRET_NARGS,  NRET_NARGS, false, false, false}},
  {"fprintf",    {NRET_YARGS,  NRET_YNARGS, NRET_NARGS, false, false, false}},
  {"sprintf",    {NRET_YARGS,  NRET_YNARGS, NRET_NARGS, false, false, false}},
  {"snprintf",   {NRET_YARGS,  NRET_YNARGS, NRET_NARGS, false, false, false}},
  {"fprintf",    {NRET_YARGS,  NRET_YNARGS, NRET_NARGS, false, false, false}},
  {"puts",       {NRET_YARGS,  NRET_NARGS,  NRET_NARGS, false, false, false}},
  {"putc",       {NRET_NARGS,  NRET_NARGS,  NRET_NARGS, false, false, false}},
  {"putchar",    {NRET_NARGS,  NRET_NARGS,  NRET_NARGS, false, false, false}},
  {"fputs",      {NRET_YARGS,  NRET_NYARGS, NRET_NARGS, false, false, false}},
  {"fputc",      {NRET_YARGS,  NRET_NYARGS, NRET_NARGS, false, false, false}},


  {"calloc",     {NRET_NARGS, YRET_NARGS, YRET_NARGS,  false, false, false}},
  {"malloc",     {NRET_NARGS, YRET_NARGS, YRET_NARGS,  false, false, false}},
  {"valloc",     {NRET_NARGS, YRET_NARGS, YRET_NARGS,  false, false, false}},
  {"memalign",   {NRET_NARGS, YRET_NARGS, YRET_NARGS,  false, false, false}},
  {"realloc",    {NRET_NARGS, YRET_NARGS, YRET_YNARGS, false,  true,  true}},
  {"free",       {NRET_NARGS, NRET_NARGS, NRET_YNARGS,  false, false, false}},
  
  {"strdup",     {NRET_YARGS, YRET_NARGS, YRET_NARGS,  false, true, false}},
  {"wcsdup",     {NRET_YARGS, YRET_NARGS, YRET_NARGS,  false, true, false}},

  {"atoi",       {NRET_YARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"atof",       {NRET_YARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"atol",       {NRET_YARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"atoll",      {NRET_YARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"atoq",       {NRET_YARGS, NRET_NARGS, NRET_NARGS, false, false, false}},

  {"memcmp",     {NRET_YARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"strcmp",     {NRET_YARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"wcscmp",     {NRET_YARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"strncmp",    {NRET_YARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"wcsncmp",    {NRET_YARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"strcasecmp", {NRET_YARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"wcscasecmp", {NRET_YARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"strncasecmp",{NRET_YARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"wcsncasecmp",{NRET_YARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"strlen",     {NRET_YARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"wcslen",     {NRET_YARGS, NRET_NARGS, NRET_NARGS, false, false, false}},

  {"memchr",     {YRET_YARGS, NRET_NARGS, NRET_NARGS, false, true, true}},
  {"wmemchr",    {YRET_YARGS, NRET_NARGS, NRET_NARGS, false, true, true}},
  {"memrchr",    {YRET_YARGS, NRET_NARGS, NRET_NARGS, false, true, true}},
  {"strchr",     {YRET_YARGS, NRET_NARGS, NRET_NARGS, false, true, true}},
  {"wcschr",     {YRET_YARGS, NRET_NARGS, NRET_NARGS, false, true, true}},
  {"strrchr",    {YRET_YARGS, NRET_NARGS, NRET_NARGS, false, true, true}},
  {"wcsrchr",    {YRET_YARGS, NRET_NARGS, NRET_NARGS, false, true, true}},
  {"strchrhul",  {YRET_YARGS, NRET_NARGS, NRET_NARGS, false, true, true}},
  {"strcat",     {YRET_YARGS, YRET_YARGS, NRET_NARGS,  true, true, true}},
  {"strncat",    {YRET_YARGS, YRET_YARGS, NRET_NARGS,  true, true, true}},

  {"strcpy",     {YRET_YARGS, YRET_YARGS, NRET_NARGS, true, true, true}},
  {"wcscpy",     {YRET_YARGS, YRET_YARGS, NRET_NARGS, true, true, true}},
  {"strcpy",     {YRET_YARGS, YRET_YARGS, NRET_NARGS, true, true, true}},
  {"wcsncpy",    {YRET_YARGS, YRET_YARGS, NRET_NARGS, true, true, true}},


  {"fwrite",     {NRET_YARGS, NRET_NYARGS, NRET_NARGS, false, false, false}},
  {"fread",      {NRET_NYARGS, NRET_YARGS, NRET_NARGS, false, false, false}},
  {"fflush",     {NRET_YARGS,  NRET_YARGS, NRET_NARGS, false, false, false}},
  {"fclose",     {NRET_YARGS,  NRET_YARGS, NRET_NARGS, false, false, false}},
  {"fopen",      {NRET_YARGS,  YRET_NARGS, YRET_NARGS, false, false, false}},
  {"fileno",     {NRET_YARGS,  NRET_NARGS, NRET_NARGS, false, false, false}},
  {"unlink",     {NRET_YARGS,  NRET_NARGS, NRET_NARGS, false, false, false}},

  {"perror",     {NRET_YARGS,  NRET_NARGS, NRET_NARGS, false, false, false}},

  // SAFECode Intrinsics
  {"sc.lscheck", {NRET_NARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"sc.lscheckui", {NRET_NARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"sc.lscheckalign", {NRET_NARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"sc.lscheckalignui", {NRET_NARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"sc.pool_register_stack", {NRET_NARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"sc.pool_unregister_stack", {NRET_NARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"sc.pool_register_global", {NRET_NARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"sc.pool_unregister_global", {NRET_NARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"sc.pool_register", {NRET_NARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"sc.pool_unregister", {NRET_NARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
  {"sc.get_actual_val", {NRET_NARGS, NRET_NARGS, NRET_NARGS, false, true, false}},

#if 0
  {"remove",     {false, false, false,  true, false, false, false, false, false}},
  {"unlink",     {false, false, false,  true, false, false, false, false, false}},
  {"rename",     {false, false, false,  true, false, false, false, false, false}},
  {"memcmp",     {false, false, false,  true, false, false, false, false, false}},
  {"execl",      {false, false, false,  true, false, false, false, false, false}},
  {"execlp",     {false, false, false,  true, false, false, false, false, false}},
  {"execle",     {false, false, false,  true, false, false, false, false, false}},
  {"execv",      {false, false, false,  true, false, false, false, false, false}},
  {"execvp",     {false, false, false,  true, false, false, false, false, false}},
  {"chmod",      {false, false, false,  true, false, false, false, false, false}},
  {"puts",       {false, false, false,  true, false, false, false, false, false}},
  {"write",      {false, false, false,  true, false, false, false, false, false}},
  {"open",       {false, false, false,  true, false, false, false, false, false}},
  {"create",     {false, false, false,  true, false, false, false, false, false}},
  {"truncate",   {false, false, false,  true, false, false, false, false, false}},
  {"chdir",      {false, false, false,  true, false, false, false, false, false}},
  {"mkdir",      {false, false, false,  true, false, false, false, false, false}},
  {"rmdir",      {false, false, false,  true, false, false, false, false, false}},
  {"read",       {false, false, false, false,  true, false, false, false, false}},
  {"pipe",       {false, false, false, false,  true, false, false, false, false}},
  {"wait",       {false, false, false, false,  true, false, false, false, false}},
  {"time",       {false, false, false, false,  true, false, false, false, false}},
  {"getrusage",  {false, false, false, false,  true, false, false, false, false}},
  {"memmove",    {false,  true, false,  true,  true, false,  true,  true,  true}},
  {"bcopy",      {false, false, false,  true,  true, false,  true, false,  true}},
  {"strcpy",     {false,  true, false,  true,  true, false,  true,  true,  true}},
  {"strncpy",    {false,  true, false,  true,  true, false,  true,  true,  true}},
  {"memccpy",    {false,  true, false,  true,  true, false,  true,  true,  true}},
  {"wcscpy",     {false,  true, false,  true,  true, false,  true,  true,  true}},
  {"wcsncpy",    {false,  true, false,  true,  true, false,  true,  true,  true}},
  {"wmemccpy",   {false,  true, false,  true,  true, false,  true,  true,  true}},
  {"getcwd",     { true,  true,  true,  true,  true,  true, false,  true,  true}},
#endif
  // C++ functions, as mangled on linux gcc 4.2
  // operator new(unsigned long)
  {"_Znwm",      {NRET_NARGS, YRET_NARGS, YRET_NARGS,  false, false, false}},
  // operator new[](unsigned long)
  {"_Znam",      {NRET_NARGS, YRET_NARGS, YRET_NARGS,  false, false, false}},
  // operator delete(void*)
  {"_ZdlPv",     {NRET_NARGS, NRET_NARGS, NRET_YNARGS,  false, false, false}},
  // operator delete[](void*)
  {"_ZdaPv",     {NRET_NARGS, NRET_NARGS, NRET_YNARGS,  false, false, false}},
  // Terminate the list of special functions recognized by this pass
  {0,            {NRET_NARGS, NRET_NARGS, NRET_NARGS, false, false, false}},
};

//
// Method: eraseCallsTo()
//
// Description:
//  This method removes the specified function from DSCallsites within the
//  specified function.  We do not do anything with call sites that call this
//  function indirectly (for which there is not much point as we do not yet
//  know the targets of indirect function calls).
//
void
StdLibDataStructures::eraseCallsTo(Function* F) {
  for (Value::use_iterator ii = F->use_begin(), ee = F->use_end();
       ii != ee; ++ii)
    if (CallInst* CI = dyn_cast<CallInst>(ii))
      if (CI->getOperand(0) == F) {
        DSGraph* Graph = getDSGraph(*CI->getParent()->getParent());
        //delete the call
        DEBUG(errs() << "Removing " << F->getNameStr() << " from " 
	      << CI->getParent()->getParent()->getNameStr() << "\n");
        Graph->removeFunctionCalls(*F);
      }
}

//
// Function: processRuntimeCheck()
//
// Description:
//  Modify a run-time check so that its return value has the same DSNode as the
//  checked pointer.
//
void
StdLibDataStructures::processRuntimeCheck (Module & M, std::string name) {
  //
  // Get a pointer to the function.
  //
  Function* F = M.getFunction (name);

  //
  // If the function doesn't exist, then there is no work to do.
  //
  if (!F) return;

  //
  // Scan through all direct calls to the function (there should only be direct
  // calls) and process each one.
  //
  for (Value::use_iterator ii = F->use_begin(), ee = F->use_end();
       ii != ee; ++ii) {
    if (CallInst* CI = dyn_cast<CallInst>(ii)) {
      if (CI->getOperand(0) == F) {
        DSGraph* Graph = getDSGraph(*CI->getParent()->getParent());
        DSNodeHandle RetNode = Graph->getNodeForValue(CI);
        DSNodeHandle ArgNode = Graph->getNodeForValue(CI->getOperand(2));
        RetNode.mergeWith(ArgNode);
      }
    }
  }

  return;
}

bool
StdLibDataStructures::runOnModule (Module &M) {
  //
  // Get the results from the local pass.
  //
  init (&getAnalysis<LocalDataStructures>(), false, true, false, false);

  //
  // Fetch the DSGraphs for all defined functions within the module.
  //
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) 
    if (!I->isDeclaration())
      getOrCreateGraph(&*I);

  //
  // Erase direct calls to functions that don't return a pointer and are marked
  // with the readnone annotation.
  //
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) 
    if (I->isDeclaration() && I->doesNotAccessMemory() &&
        !isa<PointerType>(I->getReturnType()))
      eraseCallsTo(I);

  //
  // Erase direct calls to external functions that are not varargs, do not
  // return a pointer, and do not take pointers.
  //
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) 
    if (I->isDeclaration() && !I->isVarArg() &&
        !isa<PointerType>(I->getReturnType())) {
      bool hasPtr = false;
      for (Function::arg_iterator ii = I->arg_begin(), ee = I->arg_end();
           ii != ee;
           ++ii)
        if (isa<PointerType>(ii->getType())) {
          hasPtr = true;
          break;
        }
      if (!hasPtr)
        eraseCallsTo(I);
    }

  //
  // Scan through the function summaries and process functions by summary.
  //
  for (int x = 0; recFuncs[x].name; ++x)
    if (Function* F = M.getFunction(recFuncs[x].name))
      if (F->isDeclaration()) {
        for (Value::use_iterator ii = F->use_begin(), ee = F->use_end();
             ii != ee; ++ii)
          if (CallInst* CI = dyn_cast<CallInst>(ii))
            if (CI->getOperand(0) == F) {
              DSGraph* Graph = getDSGraph(*CI->getParent()->getParent());

              //
              // Set the read, write, and heap markers on the return value
              // as appropriate.
              //
              if (recFuncs[x].action.read[0])
                Graph->getNodeForValue(CI).getNode()->setReadMarker();
              if (recFuncs[x].action.write[0])
                Graph->getNodeForValue(CI).getNode()->setModifiedMarker();
              if (recFuncs[x].action.heap[0])
                Graph->getNodeForValue(CI).getNode()->setHeapMarker();

              //
              // Set the read, write, and heap markers on the actual arguments
              // as appropriate.
              //
              for (unsigned y = 1; y < CI->getNumOperands(); ++y)
                if (recFuncs[x].action.read[y])
                  if (isa<PointerType>(CI->getOperand(y)->getType()))
                    if (DSNode * Node=Graph->getNodeForValue(CI->getOperand(y)).getNode())
                      Node->setReadMarker();
              for (unsigned y = 1; y < CI->getNumOperands(); ++y)
                if (recFuncs[x].action.write[y])
                  if (isa<PointerType>(CI->getOperand(y)->getType()))
                    if (DSNode * Node=Graph->getNodeForValue(CI->getOperand(y)).getNode())
                      Node->setModifiedMarker();
              for (unsigned y = 1; y < CI->getNumOperands(); ++y)
                if (recFuncs[x].action.heap[y])
                  if (isa<PointerType>(CI->getOperand(y)->getType()))
                    if (DSNode * Node=Graph->getNodeForValue(CI->getOperand(y)).getNode())
                      Node->setHeapMarker();

              //
              // Merge the DSNoes for return values and parameters as
              // appropriate.
              //
              std::vector<DSNodeHandle> toMerge;
              if (recFuncs[x].action.mergeWithRet)
                toMerge.push_back(Graph->getNodeForValue(CI));
              if (recFuncs[x].action.mergeAllArgs || recFuncs[x].action.mergeWithRet)
                for (unsigned y = 1; y < CI->getNumOperands(); ++y)
                  if (isa<PointerType>(CI->getOperand(y)->getType()))
                    toMerge.push_back(Graph->getNodeForValue(CI->getOperand(y)));
              for (unsigned y = 1; y < toMerge.size(); ++y)
                toMerge[0].mergeWith(toMerge[y]);

              //
              // Collapse (fold) the DSNode of the return value and the actual
              // arguments if directed to do so.
              //
              if (recFuncs[x].action.collapse) {
                if (isa<PointerType>(CI->getType()))
                  Graph->getNodeForValue(CI).getNode()->foldNodeCompletely();
                for (unsigned y = 1; y < CI->getNumOperands(); ++y)
                  if (isa<PointerType>(CI->getOperand(y)->getType()))
                    if (DSNode * Node=Graph->getNodeForValue(CI->getOperand(y)).getNode())
                      Node->foldNodeCompletely();
              }
            }

        //
        // Pretend that this call site does not call this function anymore.
        //
        eraseCallsTo(F);
      }
  
  //
  // Merge return values and checked pointer values for SAFECode run-time
  // checks.
  //
  processRuntimeCheck (M, "sc.boundscheck");
  processRuntimeCheck (M, "sc.boundscheckui");
  processRuntimeCheck (M, "sc.exactcheck2");

  return false;
}
