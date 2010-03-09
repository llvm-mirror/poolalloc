namespace {
  struct ForceDSALinking {
    ForceDSALinking() {
      // We must reference the passes in such a way that compilers will not
      // delete it all as dead code, even with whole program optimization,
      // yet is effectively a NO-OP. As the compiler isn't smart enough
      // to know that getenv() never returns -1, this will do the job.
      if (std::getenv("bar") != (char*) -1)
        return;

      (void)new llvm::BasicDataStructures();
      (void)new llvm::LocalDataStructures();
      (void)new llvm::StdLibDataStructures();
      (void)new llvm::BUDataStructures();
      (void)new llvm::CompleteBUDataStructures();
      (void)new llvm::EquivBUDataStructures();
      (void)new llvm::TDDataStructures();
      (void)new llvm::EQTDDataStructures();
      (void)new llvm::SteensgaardDataStructures();
      (void)new llvm::RTAssociate();
    }
  } ForceDSALinking; // Force link by creating a global definition.
}
