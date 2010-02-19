//===-- EntryPointAnalysis.h - Entry point Finding Pass -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is a general way of finding entry points in a system.  Simple programs
// will use the main version.  Libraries and OS kernels can have more
// specialized versions.
//
//===----------------------------------------------------------------------===//

#ifndef _ENTRYPOINTANALYSIS_H
#define	_ENTRYPOINTANALYSIS_H

namespace llvm {
class Function;
}

class EntryPointAnalysis {
public:
  static char ID;
  EntryPointAnalysis() {}
  virtual ~EntryPointAnalysis();

  virtual bool isEntryPoint(const llvm::Function* F) const = 0;
};



#endif	/* _ENTRYPOINTANALYSIS_H */

