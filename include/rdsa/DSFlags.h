class DSFlags {
  
 public:
  enum NodeTy {
    ShadowNode         = 0,         // Nothing is known about this node...
    AllocaNode         = 1 <<  0,   // This node was allocated with alloca
    HeapNode           = 1 <<  1,   // This node was allocated with malloc
    GlobalNode         = 1 <<  2,   // This node was allocated by a global
    ExternGlobalNode   = 1 <<  3,   // Tyis node was allocated by an extern global
    FunctionNode       = 1 <<  4,   // This node contains a function
    ExternFunctionNode = 1 <<  5,   // This node contains an extern func
    IntToPtrNode       = 1 <<  7,   // This node comes from an int cast
    PtrToIntNode       = 1 <<  8,   // This node escapes to an int cast
    EscapedNode        = 1 <<  9,   // This node escapes to external code
    ModifiedNode       = 1 << 10,   // This node is modified in this context
    ReadNode           = 1 << 11,   // This node is read in this context
    ArrayNode          = 1 << 12,   // This node is treated like an array

    UnknownNode        = 1 << 13,   // This node points to unknown allocated memory
    IncompleteNode     = 1 << 14,   // This node may not be complete
        
    //#ifndef NDEBUG
    DeadNode           = 1 << 15    // This node is dead and should not be pointed to
    //#endif
  };
  
 private:
  /// NodeType - A union of the above bits.  "Shadow" nodes do not add any flags
  /// to the nodes in the data structure graph, so it is possible to have nodes
  /// with a value of 0 for their NodeType.
  ///
  unsigned short NodeType;
  
 public:
  /// maskNodeTypes - Apply a mask to the node types bitfield.
  ///
  void maskFlags(unsigned short Mask) {
    NodeType &= Mask;
  }
  
  void mergeFlags(unsigned short RHS) {
    NodeType |= RHS;
  }

  unsigned short getFlags() const {
    return NodeType;
  }

  bool isAllocaNode()         const { return NodeType & AllocaNode;       }
  bool isHeapNode()           const { return NodeType & HeapNode;         }
  bool isGlobalNode()         const { return NodeType & GlobalNode;       }
  bool isExternGlobalNode()   const { return NodeType & ExternGlobalNode; }
  bool isFunctionNode()       const { return NodeType & FunctionNode;     }
  bool isExternFunctionNode() const { return NodeType & FunctionNode;     }
  bool isIntToPtrNode()       const { return NodeType & IntToPtrNode;     }
  bool isPtrToIntNode()       const { return NodeType & PtrToIntNode;     }
  bool isEscapedNode()        const { return NodeType & EscapedNode;      }
  bool isModifiedNode()       const { return NodeType & ModifiedNode;     }
  bool isReadNode()           const { return NodeType & ReadNode;         }
  bool isArrayNode()          const { return NodeType & ArrayNode;        }
  bool isUnknownNode()        const { return NodeType & UnknownNode;      }
  bool isIncompleteNode()     const { return NodeType & IncompleteNode;   }
  bool isCompleteNode()       const { return !isIncompleteNode();         }
  bool isDeadNode()           const { return NodeType & DeadNode;         }

  bool isComputedUnknownNode() const {
    return isIntToPtrNode();
  }

  void setAllocaNode()         { NodeType |= AllocaNode; }
  void setHeapNode()           { NodeType |= HeapNode; }
  void setGlobalNode()         { NodeType |= GlobalNode; }
  void setExternGlobalNode()   { NodeType |= ExternGlobalNode; }
  void setFunctionNode()       { NodeType |= FunctionNode; }
  void setExternFunctionNode() { NodeType |= ExternFunctionNode; }
  void setIntToPtrNode()       { NodeType |= IntToPtrNode; }
  void setPtrToIntNode()       { NodeType |= PtrToIntNode; }
  void setEscapedNode()        { NodeType |= EscapedNode; }
  void setModifiedNode()       { NodeType |= ModifiedNode; }
  void setReadNode()           { NodeType |= ReadNode; }
  void setArrayNode()          { NodeType |= ArrayNode; }
  
  void setUnknownNode()        { NodeType |= UnknownNode; }
  void setIncompleteNode()     { NodeType |= IncompleteNode; }
  void setDeadNode()           { NodeType |= DeadNode; }
  
  // template to avoid std::string include
  template<class STR>
  void appendString(STR& str) const {
    if (isAllocaNode()) {
      str += "S";
    }
    if (isHeapNode()) {
      str += "H";
    }
    if (isGlobalNode()) {
      str += "Gl";
    }
    if (isExternGlobalNode()) {
      str += "Ge";
    }
    if (isFunctionNode()) {
      str += "Fl";
    }
    if (isExternFunctionNode()) {
      str += "Fe";
    }
    if (isIntToPtrNode()) {
      str += "P";
    }
    if (isPtrToIntNode()) {
      str += "2";
    }
    if (isEscapedNode()) {
      str += "E";
    }
    if (isModifiedNode()) {
      str += "M";
    }
    if (isReadNode()) {
      str += "R";
    }
    if (isArrayNode()) {
      str += "A";
    }
    if (isUnknownNode()) {
      str += "U";
    }
    if (isIncompleteNode()) {
      str += "I";
    }
    if (isDeadNode()) {
      str += "<dead>";
    }
  }

  DSFlags() :NodeType(0) {}
};
