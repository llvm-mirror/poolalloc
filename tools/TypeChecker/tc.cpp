//===---- tc - Automatic Type Checking Compiler Tool --------------------===//
//
//                     Automatic Pool Allocation Project
//
// This file was developed by the LLVM research group and is distributed
// under the University of Illinois Open Source License. See LICENSE.TXT for
// details.
//
//===--------------------------------------------------------------------===//
//
// This program is a tool to run the Automatic Type Checker passes on a
// bytecode input file.
//
//===--------------------------------------------------------------------===//

#include "llvm/Config/config.h"
#include "llvm/Module.h"
#include "llvm/LLVMContext.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/FileUtilities.h"
#include "llvm/DataLayout.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/System/Signals.h"
#include "llvm/Support/raw_os_ostream.h"

#include "assistDS/TypeChecks.h"
#include "assistDS/TypeChecksOpt.h"

#include <fstream>
#include <iostream>
#include <memory>

using namespace llvm;

// General options for sc.
static cl::opt<std::string>
InputFilename(cl::Positional, cl::desc("<input bytecode>"), cl::init("-"));

static cl::opt<std::string>
OutputFilename("o", cl::desc("Output filename"), cl::value_desc("filename"));

static cl::opt<bool> Force("f", cl::desc("Overwrite output files"));

static cl::opt<bool>
EnableTypeSafetyOpts("enable-typesafety-opt",
                      cl::init(false),
                      cl::desc("Disable type-safety optimizations"));

// GetFileNameRoot - Helper function to get the basename of a filename.
static inline std::string
GetFileNameRoot(const std::string &InputFilename) {
  std::string IFN = InputFilename;
  std::string outputFilename;
  int Len = IFN.length();
  if ((Len > 2) &&
      IFN[Len-3] == '.' && IFN[Len-2] == 'b' && IFN[Len-1] == 'c') {
    outputFilename = std::string(IFN.begin(), IFN.end()-3); // s/.bc/.s/
  } else {
    outputFilename = IFN;
  }
  return outputFilename;
}


// main - Entry point for the sc compiler.
//
int main(int argc, char **argv) {

  cl::ParseCommandLineOptions(argc, argv, " llvm system compiler\n");
  sys::PrintStackTraceOnErrorSignal();

  // Load the module to be compiled...
  std::auto_ptr<Module> M;
  std::string ErrorMessage;
  if (MemoryBuffer *Buffer
      = MemoryBuffer::getFileOrSTDIN(InputFilename, &ErrorMessage)) {
    M.reset(ParseBitcodeFile(Buffer, getGlobalContext(), &ErrorMessage));
    delete Buffer;
  }

  if (M.get() == 0) {
    std::cerr << argv[0] << ": bytecode didn't read correctly.\n";
    return 1;
  }

  // Build up all of the passes that we want to do to the module...
  PassManager Passes;

  Passes.add(new DataLayout(M.get()));

  // Currently deactiviated
  Passes.add(new TypeChecks());
  if(EnableTypeSafetyOpts)
    Passes.add(new TypeChecksOpt());

  // Verify the final result
  Passes.add(createVerifierPass());

  // Figure out where we are going to send the output...
  raw_fd_ostream *Out = 0;
  std::string error;
  if (OutputFilename != "") {
    if (OutputFilename != "-") {
      // Specified an output filename?
      if (!Force && std::ifstream(OutputFilename.c_str())) {
        // If force is not specified, make sure not to overwrite a file!
        std::cerr << argv[0] << ": error opening '" << OutputFilename
          << "': file exists!\n"
          << "Use -f command line argument to force output\n";
        return 1;
      }
      Out = new raw_fd_ostream (OutputFilename.c_str(), error);

      // Make sure that the Out file gets unlinked from the disk if we get a
      // SIGINT
      sys::RemoveFileOnSignal(sys::Path(OutputFilename));
    } else {
      Out = new raw_stdout_ostream();
    }
  } else {
    if (InputFilename == "-") {
      OutputFilename = "-";
      Out = new raw_stdout_ostream();
    } else {
      OutputFilename = GetFileNameRoot(InputFilename);

      OutputFilename += ".abc.bc";
    }

    if (!Force && std::ifstream(OutputFilename.c_str())) {
      // If force is not specified, make sure not to overwrite a file!
      std::cerr << argv[0] << ": error opening '" << OutputFilename
        << "': file exists!\n"
        << "Use -f command line argument to force output\n";
      return 1;
    }

    Out = new raw_fd_ostream(OutputFilename.c_str(), error);
    if (error.length()) {
      std::cerr << argv[0] << ": error opening " << OutputFilename << "!\n";
      delete Out;
      return 1;
    }

    // Make sure that the Out file gets unlinked from the disk if we get a
    // SIGINT
    sys::RemoveFileOnSignal(sys::Path(OutputFilename));
  }

  // Add the writing of the output file to the list of passes
  Passes.add (createBitcodeWriterPass(*Out));

  // Run our queue of passes all at once now, efficiently.
  Passes.run(*M.get());

  // Delete the ostream
  delete Out;

  return 0;
}

