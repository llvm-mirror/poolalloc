//===- DSTest.cpp - Code for quering DSA results for testing --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a few basic operations that allow tests to query
// properties of the DSGraph, with an emphasis on making results something
// that is can be easily verified by 'grep' or similar basic commands.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "dsgraph-test"

#include "dsa/DataStructure.h"
#include "dsa/DSGraph.h"
#include "dsa/DSNode.h"
#include "llvm/Module.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ValueSymbolTable.h"
#include "llvm/Assembly/Writer.h"
using namespace llvm;

namespace {
  cl::list<std::string> PrintNodesForValues("print-node-for-value",
      cl::CommaSeparated, cl::ReallyHidden);
  cl::opt<bool> OnlyPrintFlags("print-only-flags", cl::ReallyHidden);
  cl::opt<bool> OnlyPrintValues("print-only-values", cl::ReallyHidden);
  cl::opt<bool> OnlyPrintTypes("print-only-types", cl::ReallyHidden);
  //Test if all mentioned values are in the same node
  cl::list<std::string> CheckNodesSame("check-same-node",
      cl::CommaSeparated, cl::ReallyHidden);
}

/// NodeValue -- represents a particular node in a DSGraph
/// constructed from a serialized string representation of a value
/// 
/// FIXME: Make this integrated into cl parsing, as mentioned:
///   http://llvm.org/docs/CommandLine.html#customparser
///
/// Supported formats (so far)
/// graph:value[:offset]*
/// Examples:
/// "value"  specifies 'value' in the globals graph
/// "func:value" specifies 'value' in graph for function 'func'
/// "func:value:0" the node pointed to at offset 0 from the above
/// "func:value:0:1" the node pointed to at offset 1 from the above
/// ..etc
/// We are also robust to "@value" and "@func" notation for convenience
/// FIXME: Support querying special nodes like return nodes, VANodes, etc
class NodeValue {
  // Containing Function, if applicable.
  Function *F;
  // Value in that graph's scalarmap that we base off of
  // (note that the NH we have below could be indexed a few times
  // from this value, only corresponds directly if no offsets)
  Value *V;
  // DSNodehandle
  DSNodeHandle NH;

  // String version (that we were given)
  StringRef serialized;

  // Parsed list of offsets
  typedef SmallVector<unsigned,3> OffsetVectorTy;
  OffsetVectorTy offsets;

  NodeValue() {};
  void operator=(const NodeValue&);
  NodeValue(const NodeValue&);

  void initialize(const Module *M, const DataStructures *DS) {
    parseValue(M);
    assert(V && "Failed to parse value?");
    if (isa<GlobalValue>(V)) {
      DSGraph *G = DS->getGlobalsGraph();
      assert(G->hasNodeForValue(V) && "Node not in specified graph!");
      NH = G->getNodeForValue(V);
    } else {
      assert(F && "No function?");
      DSGraph *G = DS->getDSGraph(*F);
      assert(G->hasNodeForValue(V) && "Node not in specified graph!");
      NH = G->getNodeForValue(V);
    }
    // Handle offsets, if any
    // For each offset in the offsets vector, follow the link at that offset
    for (OffsetVectorTy::const_iterator I = offsets.begin(), E = offsets.end();
        I != E; ++I ) {
      assert(!NH.isNull() && "Null NodeHandle?");
      assert(NH.hasLink(*I) && "Handle doesn't have link?");
      // Follow the offset
      NH = NH.getLink(*I);
    }
  }

  /// parseValue -- sets value for the string we were constructed on,
  /// using the provided module as the context to find the value
  void parseValue(const Module *M) {
    unsigned count = serialized.count(':');
    if (count == 0) {
      // Global case
      // format: "[@]value"
      StringRef globalName = stripAtIfRequired(serialized);

      V = M->getNamedValue(globalName);
      assert(V && "Unable to find specified global!");
    } else { // count >= 1
      // Function-specific case
      // format: "[@]func:value"
      // format: "[@]func:value:offset0:offset1:offset2"

      std::pair<StringRef,StringRef> split = serialized.split(':');
      StringRef func = stripAtIfRequired(split.first);
      StringRef value = split.second;

      if (count > 1) {
        //If we have more semicolons, split them off to get the value and offsets
        std::pair<StringRef,StringRef> tmp = value.split(':');
        value = tmp.first;
        StringRef offStrings = tmp.second;

        //Small detour--parse offsets into vector
        unsigned offset;
        while(offStrings.count(':') > 0) {
          tmp = offStrings.split(':');
          assert(!tmp.first.getAsInteger(0,offset) && "failed to parse offset!");
          offsets.push_back(offset);
          offStrings = tmp.second;
        }
        assert(!offStrings.getAsInteger(0,offset) && "failed to parse offset!");
        offsets.push_back(offset);
      }
      // Now back to your regularly scheduled programming...

      // First, find the function
      F = M->getFunction(func);
      assert(F && "Unable to find function specified!");

      // Now we try to find the value...
      // FIXME: This only works for named values, things like "%1" don't work.
      // That might not be a deal breaker, but should be clear.
      V = F->getValueSymbolTable().lookup(value);

      assert(V && "Unable to find value in specified function!");

    }

    assert(V && "Parsing value failed!");
  }

  /// stripAtIfRequired -- removes the leading '@' character if one exists
  ///
  StringRef stripAtIfRequired(StringRef v) {
    if (!v.startswith("@"))
        return v;

    assert(v.size() > 1 && "String too short");

    return v.substr(1);
  }
public:
  /// Constructor (from string)
  NodeValue(std::string & raw, const Module * M, const DataStructures *DS)
    : F(NULL), V(NULL), serialized(raw) {
      initialize(M,DS);
      assert(V && NH.getNode() && "Parse failed!");
    };

  /// Accessors
  DSNodeHandle & getNodeH() { return NH;                          }
  DSGraph  * getGraph()     { return getNode()->getParentGraph(); }
  // FIXME: These two (value/function) aren't used presently, and furthermore
  // are a bit confusing in the context of offsets.  Make this not lame.
  Value    * getValue()     { return V;                           }
  Function * getFunction()  { return F;                           }

  //Helper to fetch the node from the nodehandle
  DSNode * getNode() {
    assert(NH.getNode() && "NULL node?");
    return NH.getNode();
  }
};

/// printAllValuesForNode -- prints all values for a given node, without a newline
/// (meant to be a helper)
static void printAllValuesForNode(llvm::raw_ostream &O, NodeValue &NV) {
  // We only consider other values that are in the graph
  // containing the specified node (by design)

  // Look for values that have an equivalent NH
  DSNodeHandle &NH = NV.getNodeH();
  const DSGraph::ScalarMapTy &SM = NV.getGraph()->getScalarMap();
  bool first = true;

  for (DSGraph::ScalarMapTy::const_iterator I = SM.begin(), E = SM.end();
      I != E; ++I )
    if (NH == I->second) {
      //Found one!
      const Value *V = I->first;

      //Print them out, separated by commas
      if (!first) O << ",";
      first = false;

      // Print out name, if it has one.
      // FIXME: Get "%0, "%1", naming like the .ll has?
      if (V->hasName())
        O << V->getName();
      else
        O << "<tmp>";
    }

  //FIXME: Search globals in this graph too (not just scalarMap)?
}

// printTypesForNode --prints all the types for the given NodeValue, without a newline
// (meant to be called as a helper)
static void printTypesForNode(llvm::raw_ostream &O, NodeValue &NV) {
  DSNode *N = NV.getNode();
  Module *M = NV.getFunction()->getParent();

  if (N->isNodeCompletelyFolded()) {
    O << "Folded";
  }

  // Go through all the types, and just dump them.
  // FIXME: Lifted from Printer.cpp, probably should be shared
  if (N->type_begin() != N->type_end())
    for (DSNode::TyMapTy::const_iterator ii = N->type_begin(),
        ee = N->type_end(); ii != ee; ++ii) {
      O << ii->first << ": ";
      if (ii->second) {
        bool first = true;
        for (svset<const Type*>::const_iterator ni = ii->second->begin(),
            ne = ii->second->end(); ni != ne; ++ni) {
          if (!first) O << ",";
          WriteTypeSymbolic(O, *ni, M);
          first = false;
        }
      }
      else
        O << "VOID";
      O << " ";
    }
  else
    O << "VOID";

  if (N->isArrayNode())
    O << " array";
}

static void printFlags(llvm::raw_ostream &O, DSNode *N) {
  // FIXME: This code is lifted directly from Printer.cpp
  // Probably would be good to make this code shared...
  // Leaving it separate for now to minimize invasiveness
  if (unsigned NodeType = N->getNodeFlags()) {
    if (NodeType & DSNode::AllocaNode     ) O << "S";
    if (NodeType & DSNode::HeapNode       ) O << "H";
    if (NodeType & DSNode::GlobalNode     ) O << "G";
    if (NodeType & DSNode::UnknownNode    ) O << "U";
    if (NodeType & DSNode::IncompleteNode ) O << "I";
    if (NodeType & DSNode::ModifiedNode   ) O << "M";
    if (NodeType & DSNode::ReadNode       ) O << "R";
    if (NodeType & DSNode::ExternalNode   ) O << "E";
    if (NodeType & DSNode::IntToPtrNode   ) O << "P";
    if (NodeType & DSNode::PtrToIntNode   ) O << "2";
    if (NodeType & DSNode::VAStartNode    ) O << "V";
  }
}

/// printNodes -- print the node specified by NV
///
/// Format:
/// (can a type include '{}'s, etc?)
/// "flags:{value(s)}:{type(s)}"
///
/// Additionally, the user can specify to print just one piece
static void printNode(llvm::raw_ostream &O, NodeValue &NV) {
  assert(
      ((!OnlyPrintFlags && !OnlyPrintValues)||
      (!OnlyPrintFlags && !OnlyPrintTypes) ||
      (!OnlyPrintValues && !OnlyPrintTypes)) &&
      "Only one \"Only\" option allowed!");

  if (OnlyPrintFlags) {
    printFlags(O,NV.getNode());
  } else if (OnlyPrintValues) {
    printAllValuesForNode(O, NV);
  } else if (OnlyPrintTypes) {
    printTypesForNode(O,NV);
  } else {
    //Print all of them
    printFlags(O,NV.getNode());
    O << ":{";
    printAllValuesForNode(O, NV);
    O << "}:{";
    printTypesForNode(O,NV);
    O << "}";
  }

  O << "\n";
}

/// printTestInfo -- runs through the user-specified testing arguments (if any)
/// and prints the requested information.
void DataStructures::printTestInfo(llvm::raw_ostream &O, const Module *M) const {

  // For each node the user indicated, print the node.
  // See 'printNode' for more details.
  for (cl::list<std::string>::iterator I = PrintNodesForValues.begin(),
      E = PrintNodesForValues.end(); I != E; ++I ) {
    // Make sense of what the user gave us
    NodeValue NV(*I, M, this);
    // Print corresponding node
    printNode(O, NV);
  }

  // Verify all nodes listed in "CheckNodesSame" belong to the same node.
  cl::list<std::string>::iterator CI = CheckNodesSame.begin(),
                                  CE = CheckNodesSame.end();
  // If the user specified that a set of values should be in the same node...
  if (CI != CE) {
    // Take the first such value as the reference to compare to the others
    NodeValue NVReference(*CI++,M,this);

    // Iterate through the remaining to verify they're the same node.
    for(; CI != CE; ++CI) {
      NodeValue NV(*CI, M, this);
      assert(NVReference.getNodeH()==NV.getNodeH() && "Nodes don't match!");
    }
  }
}

