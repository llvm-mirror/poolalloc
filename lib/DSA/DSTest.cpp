//===- DSTest.cpp - Queries DSA results for testing -----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This defines various commandline options to DSA to help in regression tests.
// These options are:
// -print-node-for-value=<list>   Print the DSNodes for the given values
//   -print-only-flags              Only print Flags for the given values
//   -print-only-values             Only print the values pointed to by the given values
//   -print-only-types              Only print the types for the given values
// -check-same-node=<list>        Verify the given values' nodes were merged.
// -check-not-same-node=<list>    Verify the given values' nodes weren't merged.
// -verify-flags=<list>           Verify the given values match the flag specifications.
//
// In general a 'value' query on the DSA results looks like this:
// graph:value[:offset]*
//  Examples:
//    "value"  specifies 'value' in the globals graph
//    "func:value" specifies 'value' in graph for function 'func'
//    "func:value:0" the node pointed to at offset 0 from the above
//    "func:value:0:1" the node pointed to at offset 1 from the above
//    ..etc
//    We are also robust to "@value" and "@func" notation for convenience
// The -verify-flags option takes values in this format, but also followed
// by any number of 'flag specifiers' of the form '+flags' and '-flags',
// which indicate flags that the node should and shouldn't have.
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
  // Test if all mentioned values are in the same node (merged)
  cl::list<std::string> CheckNodesSame("check-same-node",
      cl::CommaSeparated, cl::ReallyHidden);
  // Test if all mentioned values are in distinct nodes
  cl::list<std::string> CheckNodesNotSame("check-not-same-node",
      cl::CommaSeparated, cl::ReallyHidden);
  // For each value, verify they have (or don't have) the specified flags
  cl::list<std::string> VerifyFlags("verify-flags",
      cl::CommaSeparated, cl::ReallyHidden);
  // For each value, verify that type is as given
  cl::list<std::string> CheckType("check-type",
      cl::CommaSeparated, cl::ReallyHidden);
}

/// NodeValue -- represents a particular node in a DSGraph
/// constructed from a serialized string representation of a value
/// 
/// FIXME: Make this integrated into cl parsing, as mentioned:
///   http://llvm.org/docs/CommandLine.html#customparser
///
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
  std::string serialized;

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

  /// stripOffsets -- strips the offsets
  /// Walks backwards, stripping offsets.
  /// Returns serialized without the offsets
  ///
  std::string stripOffsets() {
    std::vector<unsigned> offsets_reversed;
    SmallVector<StringRef,5> colonSeparated;
    StringRef serializedRef = serialized;
    serializedRef.split(colonSeparated,":");
    SmallVector<StringRef,5>::reverse_iterator I = colonSeparated.rbegin(),
      E = colonSeparated.rend();
    for(; I != E; ++I ) {
      unsigned offset;
      // If this isn't an integer (offset), then bail
      if (I->getAsInteger(0,offset))
        break;
      offsets_reversed.push_back(offset);
    }
    // Okay so we built reversed list of offsets, now put things back together

    // If we have more than 2 values left, then we have something like:
    // name1:name2:name3[:offset]*, which is no good.
    // Also, if we have *nothing* left, something is similarly wrong.
    int stringsLeft = E - I;
    assert((stringsLeft > 0) && "Node was entirely made of offsets?");
    assert((stringsLeft <= 2) && "Too many colons! (Invalid node/offset given)");

    // Now rebuild the string, without the offsets.
    std::string rebuilt = I++->str();
    for(; I != E; ++I) {
      rebuilt = I->str() + ":" + rebuilt;
    }

    // Reverse the offsets (since we parsed backwards) and put the result
    // into the 'offsets' vector for use elsewhere.
    offsets.insert(offsets.begin(),
        offsets_reversed.rbegin(),offsets_reversed.rend());

    return rebuilt;
  }


  /// parseValue -- sets value for the string we were constructed on,
  /// using the provided module as the context to find the value
  void parseValue(const Module *M) {
    // Parse the offsets, and remove from the string
    StringRef stripped = stripOffsets();

    unsigned count = stripped.count(':');
    if (count == 0) {
      // Global case
      // format: "[@]value"
      StringRef globalName = stripAtIfRequired(stripped);

      V = M->getNamedValue(globalName);
      assert(V && "Unable to find specified global!");
    } else if (count == 1) {
      // Function-specific case
      // format: "[@]func:value"

      std::pair<StringRef,StringRef> split = stripped.split(':');
      StringRef func = stripAtIfRequired(split.first);
      StringRef value = split.second;

      // First, find the function
      F = M->getFunction(func);
      assert(F && "Unable to find function specified!");

      // Now we try to find the value...
      // FIXME: This only works for named values, things like "%1" don't work.
      // That might not be a deal breaker, but should be clear.
      V = F->getValueSymbolTable().lookup(value);

      assert(V && "Unable to find value in specified function!");

    } else {
      assert(0 && "Too many colons, offsets not stripped?");
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

  /// Helper to fetch the node from the nodehandle
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
  bool firstType = true;
  if (N->type_begin() != N->type_end())
    for (DSNode::TyMapTy::const_iterator ii = N->type_begin(),
        ee = N->type_end(); ii != ee; ++ii) {
      if (!firstType) O << "::";
      firstType = false;
      O << ii->first << ":";
      if (ii->second) {
        bool first = true;
        for (svset<const Type*>::const_iterator ni = ii->second->begin(),
            ne = ii->second->end(); ni != ne; ++ni) {
          if (!first) O << "|";
          WriteTypeSymbolic(O, *ni, M);
          first = false;
        }
      }
      else
        O << "VOID";
    }
  else
    O << "VOID";

  if (N->isArrayNode())
    O << "Array";
}

static std::string getFlags(DSNode *N) {
  std::string flags("");

  // FIXME: This code is lifted directly from Printer.cpp
  // Probably would be good to make this code shared...
  // Leaving it separate for now to minimize invasiveness
  if (unsigned NodeType = N->getNodeFlags()) {
    if (NodeType & DSNode::AllocaNode     ) flags += "S";
    if (NodeType & DSNode::HeapNode       ) flags += "H";
    if (NodeType & DSNode::GlobalNode     ) flags += "G";
    if (NodeType & DSNode::UnknownNode    ) flags += "U";
    if (NodeType & DSNode::IncompleteNode ) flags += "I";
    if (NodeType & DSNode::ModifiedNode   ) flags += "M";
    if (NodeType & DSNode::ReadNode       ) flags += "R";
    if (NodeType & DSNode::ExternalNode   ) flags += "E";
    if (NodeType & DSNode::IntToPtrNode   ) flags += "P";
    if (NodeType & DSNode::PtrToIntNode   ) flags += "2";
    if (NodeType & DSNode::VAStartNode    ) flags += "V";
  }

  return flags;
}

static void printFlags(llvm::raw_ostream &O, DSNode *N) {
  O << getFlags(N);
}

/// printNodes -- print the node specified by NV
///
/// Format:
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
    printTypesForNode(O, NV);
  } else {
    //Print all of them
    printFlags(O,NV.getNode());
    O << ":{";
    printAllValuesForNode(O, NV);
    O << "}:{";
    printTypesForNode(O, NV);
    O << "}";
  }

  O << "\n";
}


/// printNodes -- For each node the user indicated, print the node.
/// See 'printNode' for more details.
/// Returns true iff the user specified nodes to print.
///
static bool printNodes(llvm::raw_ostream &O, const Module *M, const DataStructures *DS) {
  cl::list<std::string>::iterator I = PrintNodesForValues.begin(),
                                  E = PrintNodesForValues.end();
  if (I != E) {
    for ( ; I != E; ++I ) {
      // Make sense of what the user gave us
      NodeValue NV(*I, M, DS);
      // Print corresponding node
      printNode(O, NV);
    }
    return true;
  }
  return false;
}

/// checkIfNodesAreSame -- Verify each node that the user indicated
/// should be merged, is in fact merged.
/// Returns true iff the user specified any nodes for this option.
///
static bool checkIfNodesAreSame(llvm::raw_ostream &O, const Module *M, const DataStructures *DS) {

  // Verify all nodes listed in "CheckNodesSame" belong to the same node.
  cl::list<std::string>::iterator I = CheckNodesSame.begin(),
                                  E = CheckNodesSame.end();
  // If the user specified that a set of values should be in the same node...
  if (I != E) {
    // Take the first such value as the reference to compare to the others
    NodeValue NVReference(*I++, M, DS);

    // Iterate through the remaining to verify they're the same node.
    for(; I != E; ++I) {
      NodeValue NV(*I, M, DS);
      assert(NVReference.getNodeH()==NV.getNodeH() && "Nodes don't match!");
    }
    return true;
  }

  return false;
}

/// checkIfNodesAreNotSame -- Verify each node that the user indicated
/// shouldn't be merged, wasn't merged
/// Returns true iff the user specified any nodes for this option.
///
static bool checkIfNodesAreNotSame(llvm::raw_ostream &O, const Module *M, const DataStructures *DS) {

  // Verify all nodes listed in "CheckNodesNotSame" belong to distinct nodes.
  cl::list<std::string>::iterator I = CheckNodesNotSame.begin(),
                                  E = CheckNodesNotSame.end();

  // If the user specified that a set of values should be in separate nodes...
  if (I != E) {
    // Lookup all the values
    unsigned count = E - I;
    NodeValue ** NV = new NodeValue*[count];
    for(unsigned i = 0; I != E; ++I, ++i)
      NV[i] = new NodeValue(*I, M, DS);

    //Compare all pairs to make sure they're distinct
    for(unsigned i = 0; i < count; ++i)
      for(unsigned j = i+1; j < count; ++j) {
        assert(NV[i]->getNodeH() != NV[j]->getNodeH() && "Nodes not distinct!");
      }

    for(unsigned i = 0; i < count; ++i)
      delete NV[i];
    delete [] NV;

    return true;
  }

  return false;
}

/// checkTypes -- Verify type for the given nodes.
/// Returns true iff the user specified anything for this option
///

static bool checkTypes(llvm::raw_ostream &O, const Module *M, const DataStructures *DS) {
  
  // Verify all nodes listed in "CheckType" have the same Type
  cl::list<std::string>::iterator I = CheckType.begin(),
                                  E = CheckType.end();
  // If the user specified that a set of values should be in the same node...
  if (I != E) {
    // last value is type string
    std::string typeRef = *(--E);
    //typeRef = typeRef.substr(1, typeRef.length()-2);
    // Iterate through the remaining to verify they're the same node.
    for(; I != E; ++I) {
      NodeValue NV(*I, M, DS);
      std::string *type = new std::string();
      llvm::raw_string_ostream *test= new llvm::raw_string_ostream(*type);
      
      printTypesForNode(*test, NV);
      
      assert(test->str()==typeRef && "Types don't match!");
    }
    return true;
  }
  return false;
}

/// VerifyFlags -- Verify flag properties for the given nodes.
/// This is a common enough testing process that this was added to make it simpler.
/// Returns true iff the user specified anything for this option.
///
/// This builds upon the node notation used elsewhere, and tacks on
/// node+flags, node-flags, node+flags-flags
/// Where +flags means 'this node should have these flags'
/// And -flags means 'this node should NOT have these flags'
///
static bool verifyFlags(llvm::raw_ostream &O, const Module *M, const DataStructures *DS) {
  cl::list<std::string>::iterator I = VerifyFlags.begin(),
                                  E = VerifyFlags.end();
  if (I != E) {
    for(; I != E; ++I) {
      std::string NodeFlagOption = *I;
      std::string::size_type FlagPos = NodeFlagOption.find_first_of("+-");
      if (FlagPos == std::string::npos) {
        errs() << "No flags given for option \"" << NodeFlagOption << "\"!\n";
        assert(0 && "Invalid input!");
      }
      
      // Grab the part before the flag specifiers and parse that as a node
      std::string NodeString = std::string(I->begin(),I->begin()+FlagPos);
      NodeValue NV(NodeString, M, DS);

      // Process each of the flag specifiers (+flag, or -flag)
      do {
        bool shouldHaveFlag = (NodeFlagOption[FlagPos] == '+');

        // Find the next specifier...
        std::string::size_type NextPos = NodeFlagOption.find_first_of("+-",FlagPos+1);

        // Parse out the flags for this option
        std::string FlagsListed;
        if (NextPos != std::string::npos)
          FlagsListed = std::string(I->begin()+FlagPos+1,I->begin()+NextPos);
        else
          FlagsListed = std::string(I->begin()+FlagPos+1,I->end());

        // Do the checking!
        std::string ActualFlags = getFlags(NV.getNode());
        for (std::string::iterator I = FlagsListed.begin(), E = FlagsListed.end();
            I != E; ++I ) {
          if (shouldHaveFlag)
            assert((ActualFlags.find(*I) != std::string::npos)
                && "Node doesn't have flag it should!");
          else
            assert((ActualFlags.find(*I) == std::string::npos)
                && "Node has flag it shouldn't!");
        }


        // Update FlagPos
        FlagPos = NextPos;
      } while(FlagPos != std::string::npos);
    }
    return true;
  }

  return false;

}

/// handleTest -- handles any user-specified testing options.
/// returns true iff the user specified something to test.
///
bool DataStructures::handleTest(llvm::raw_ostream &O, const Module *M) const {

  bool tested = false;

  tested |= printNodes(O,M,this);
  tested |= checkIfNodesAreSame(O,M,this);
  tested |= checkIfNodesAreNotSame(O,M,this);
  tested |= verifyFlags(O,M,this);
  tested |= checkTypes(O,M,this);

  return tested;
}

