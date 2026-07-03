                â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”         Hybrid CLI               â”‚
                â”‚   (niyah_hybrid_main.c)           â”‚
                â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”˜
                           â”‚                  â”‚
          â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”  Neural Core  â”‚    â”‚  Symbolic Reasoner  â”‚
          â”‚ (niyah_core)  â”‚    â”‚ (hybrid_reasoner)   â”‚
          â”‚               â”‚    â”‚                     â”‚
          â”‚  Transformer  â”‚    â”‚  Unification        â”‚
          â”‚  GQA Attn     â”‚    â”‚  Backward Chaining  â”‚
          â”‚  SwiGLU FFN   â”‚    â”‚  Knowledge Base     â”‚
          â”‚  RoPE         â”‚    â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
          â”‚  RMSNorm      â”‚             â”‚
          â”‚  SIMD Kernels â”‚    â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”    â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”    â”‚  Constraint Solver  â”‚
                  â”‚            â”‚  (constraint_solver) â”‚
                  â”‚            â”‚  Rational Arithmetic â”‚
          â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”  â”‚  Bounds Propagation â”‚
          â”‚   Tokenizer   â”‚    â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
          â”‚   (UTF-8)     â”‚             â”‚
          â””â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”˜    â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”            â”‚   Rule Parser       â”‚
                  â–¼            â”‚   (.nrule format)   â”‚
            â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”    â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
            â”‚  Output   â”‚               â”‚
            â”‚  Text     â”‚â—€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
            â””â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”˜        Verify & Enforce
                  â”‚
          â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â” Proof Generatorâ”‚
          â”‚  SHA-256 Hash  â”‚
          â”‚  Audit Trail   â”‚
          â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜

### Design Principles

- **Zero dependencies** â€” libc + libm only. No OpenSSL, no Boost, no BLAS.
- **Single-pool allocation** â€” one `malloc`, zero fragmentation, deterministic teardown.
- **SIMD everywhere** â€” AVX2+FMA (x86_64), NEON (aarch64), scalar fallback. Compile-time selection.
- **C11 clean** â€” compiles with `-Wall -Wextra -Werror -pedantic`. C++17 compatible headers.
- **Offline-first** â€” no network calls, no telemetry, no cloud. Fully sovereign.

---

## Features

### Neural Core (`niyah_core.c` â€” 809 lines)
- Transformer decoder with Grouped-Query Attention (GQA)
- SwiGLU feed-forward network with configurable multiplier
- Rotary Position Embeddings (RoPE) with adjustable theta
- RMSNorm (pre-attention and pre-FFN)
- KV-cache with head-major layout for optimal attention locality
- Adam optimizer with weight decay for training
- Top-p (nucleus) sampling with temperature control
- Model serialization to `.bin` format (64-byte header)

### Symbolic Reasoner (`hybrid_reasoner.c` â€” 801 lines)
- First-order logic terms: atoms, variables, compound terms
- Robinson's unification algorithm with occurs check
- Backward chaining (Prolog-style) with configurable depth limit
- Knowledge base with clause indexing
- Variable renaming for clause isolation

### Constraint Solver (`constraint_solver.c` â€” 479 lines)
- Exact rational arithmetic (int64 numerator/denominator)
- Linear inequality constraints (â‰¤, â‰¥, =)
- Bounds propagation solver with iterative tightening
- Integrates with the symbolic reasoner to prune impossible bindings

### Rule Parser (`rule_parser.c` â€” 604 lines)
- Human-readable `.nrule` format for output verification
- Recursive descent parser
- Supports: `IF`/`THEN`, `ALWAYS`, `CONTAINS`, `EQUALS`, `NOT_EQUALS`, `AND`, `REJECTED`, `MUST contain`
- Case-insensitive matching
- Sequence-level verification: generate full response â†’ decode â†’ verify â†’ re-sample if violated

### Proof Generator (`proof_generator.c` â€” 402 lines)
- Public-domain SHA-256 (FIPS 180-4) â€” no OpenSSL dependency
- Proof hash: `SHA-256(prompt â€– output â€– rule_file_contents)`
- Machine-verifiable `.proof` file format (NIYAH-PROOF-V1)
- Offline audit trail for every inference

### SIMD Kernels
| Operation | AVX2+FMA | NEON | Scalar |
|-----------|----------|------|--------|
| RMSNorm   | âœ“        | âœ“    | âœ“      |
| MatVec    | âœ“        | âœ“    | âœ“      |
| SwiGLU    | âœ“        | âœ“    | âœ“      |
| Softmax   | âœ“        | âœ“    | âœ“      |

---

## Quick Start

### Linux / macOS (GCC or Clang)

```bash
# Clone
git clone https://github.com/Khawrzm/casper_engine.git
cd casper_engine

# Build core binaries (auto-detects arch + SIMD)
bash scripts/build_gcc.sh

# Run smoke tests (neural core)
RUN_SMOKE=1 bash scripts/build_gcc.sh

# Build hybrid binary (neural + symbolic)
gcc -O2 -std=c11 -Wall -Wextra -Werror \
    Core_CPP/niyah_core.c Core_CPP/hybrid_reasoner.c \
    Core_CPP/constraint_solver.c Core_CPP/rule_parser.c \
    Core_CPP/proof_generator.c Core_CPP/niyah_hybrid_main.c \
    tokenizer.c -o niyah_hybrid -lm

# Run all 96 tests (neural + symbolic + constraints + rules + proofs)
./niyah_hybrid --smoke
# Casper Engine

**NIYAH â€” Sovereign Hybrid Neuro-Symbolic Inference Engine**

> *We are the heirs of Al-Khwarizmi â€” nothing is impossible.*

A from-scratch C11 inference and training engine that fuses a Transformer neural core with a symbolic reasoning layer, cryptographic proof generation, and constraint solving. Zero external dependencies. Runs on $35 hardware.

---

## Architecture Overview

cd C:\Users\You\casper_engine

# Unified entrypoint
.\scripts\niyah.ps1 build
.\scripts\niyah.ps1 smoke
.\scripts\niyah.ps1 train Data_Training\sovereign_knowledge.txt 3 0.001
# Standalone trainer
gcc -O2 -std=c11 Core_CPP/niyah_core.c Core_CPP/niyah_train.c \
    tokenizer.c -o niyah_train -lm

# Train on corpus (3 epochs, lr=0.001)
./niyah_train Data_Training\sovereign_knowledge.txt 3 0.001

# Save model
./niyah_hybrid --model niyah_trained.bin --save

Hybrid Inference
The hybrid engine generates text with the neural Transformer, then verifies the output against symbolic rules before returning it.

Flow
Encode â€” tokenize prompt (UTF-8, Arabic-aware)
Generate â€” autoregressive forward pass with KV-cache
Decode â€” detokenize to candidate text
Verify â€” check against .nrule constraints
Re-sample â€” if violated, adjust seed and retry (up to max_retries)
Prove â€” compute SHA-256 proof hash over (prompt, output, rules)
Return â€” verified output + proof
Usage
# Interactive mode with rules
./niyah_hybrid --model model.bin --rules safety.nrule --interactive

# Verify a proof file
./niyah_hybrid --verify-proof response.proof
// Comments start with //
rule: "IF question CONTAINS 'password' THEN REJECTED"
rule: "IF output CONTAINS 'hack' THEN answer = 'I cannot help with that.'"
rule: "ALWAYS output MUST contain 'bismillah'"
Proof File Format (.proof)
NIYAH-PROOF-V1
hash: a1b2c3d4...
prompt_hash: e5f6a7b8...
output_hash: c9d0e1f2...
rules_hash: 34567890...
timestamp: 2026-04-09T12:00:00Z
NIYAH-PROOF-V1
hash: a1b2c3d4...
prompt_hash: e5f6a7b8...
output_hash: c9d0e1f2...
rules_hash: 34567890...
timestamp: 2026-04-09T12:00:00Z
casper_engine/
â”œâ”€â”€ Core_CPP/                    # Engine source code
â”‚   â”œâ”€â”€ niyah_core.h             # Public API header (189 lines)
â”‚   â”œâ”€â”€ niyah_core.c             # Neural Transformer engine (809 lines)
â”‚   â”œâ”€â”€ hybrid_reasoner.h/c      # Symbolic reasoner (949 lines)
â”‚   â”œâ”€â”€ constraint_solver.h/c    # Linear constraint solver (575 lines)
â”‚   â”œâ”€â”€ rule_parser.h/c          # .nrule format parser (723 lines)
â”‚   â”œâ”€â”€ proof_generator.h/c      # SHA-256 + proof system (466 lines)
â”‚   â”œâ”€â”€ niyah_hybrid_main.c      # Hybrid CLI entrypoint (434 lines)
â”‚   â”œâ”€â”€ niyah_main.c             # Neural-only smoke test (18 lines)
â”‚   â”œâ”€â”€ niyah_train.c            # Standalone trainer (186 lines)
â”‚   â””â”€â”€ bench_niyah.c            # Benchmarking harness (223 lines)
â”œâ”€â”€ tokenizer.c                  # UTF-8 tokenizer with encode/decode (186 lines)
â”œâ”€â”€ Data_Training/               # Training datasets
â”œâ”€â”€ Math_ASM/                    # Assembly experiments (AVX)
â”œâ”€â”€ UI_CSharp/                   # Optional C# manager UI
â””â”€â”€ scripts/                     # Build & automation
    â”œâ”€â”€ build_gcc.sh             # GCC/Clang build (Linux/macOS)
    â”œâ”€â”€ build_msvc.ps1           # MSVC build (Windows)
    â””â”€â”€ niyah.ps1                # Unified PowerShell wrapper
Total codebase: ~4,750 lines of C11 (excluding tests embedded in source).

Building
Requirements
C11 compiler: GCC 7+, Clang 6+, or MSVC 2019+
No external libraries required
Optional: cppcheck for static analysis
# Release (default)
gcc -O3 -std=c11 -Wall -Wextra -Werror -DNDEBUG ...

# Debug with sanitizers
gcc -O0 -g3 -std=c11 -Wall -Wextra -Werror \
    -fsanitize=address,undefined ...

# Force architecture
bash scripts/build_gcc.sh --arch arm64
bash scripts/build_gcc.sh --arch x86_64

# With static analysis
RUN_LINT=1 bash scripts/build_gcc.sh
Standalone Tests
Each subsystem compiles independently for isolated testing:
# Symbolic reasoner (21 tests)
gcc -O2 -std=c11 -Wall -Wextra -Werror \
    Core_CPP/hybrid_reasoner.c -DSYM_STANDALONE_TEST -o test_reasoner && ./test_reasoner

# Constraint solver (19 tests)
gcc -O2 -std=c11 -Wall -Wextra -Werror \
    Core_CPP/constraint_solver.c -DCSP_STANDALONE_TEST -o test_csp && ./test_csp

# Rule parser (22 tests)
gcc -O2 -std=c11 -Wall -Wextra -Werror \
    Core_CPP/rule_parser.c -DRULE_STANDALONE_TEST -o test_rules && ./test_rules

# Proof generator (11 tests, NIST SHA-256 vectors)
gcc -O2 -std=c11 -Wall -Wextra -Werror \
    Core_CPP/proof_generator.c -DPROOF_STANDALONE_TEST -o test_proof && ./test_proof

# Full suite (96 tests across all subsystems)
./niyah_hybrid --smoke
# Symbolic reasoner (21 tests)
gcc -O2 -std=c11 -Wall -Wextra -Werror \
    Core_CPP/hybrid_reasoner.c -DSYM_STANDALONE_TEST -o test_reasoner && ./test_reasoner

# Constraint solver (19 tests)
gcc -O2 -std=c11 -Wall -Wextra -Werror \
    Core_CPP/constraint_solver.c -DCSP_STANDALONE_TEST -o test_csp && ./test_csp

# Rule parser (22 tests)
gcc -O2 -std=c11 -Wall -Wextra -Werror \
    Core_CPP/rule_parser.c -DRULE_STANDALONE_TEST -o test_rules && ./test_rules

# Proof generator (11 tests, NIST SHA-256 vectors)
gcc -O2 -std=c11 -Wall -Wextra -Werror \
    Core_CPP/proof_generator.c -DPRNiyahModel *niyah_alloc(const NiyahConfig *cfg);
    void        niyah_free(NiyahModel *m);
    int         niyah_save(const NiyahModel *m, const char *path);
    int         niyah_load(NiyahModel **out, const char *path);
    float      *niyah_forward(NiyahModel *m, uint32_t token, uint32_t pos);
    uint32_t    niyah_sample(const float *logits, uint32_t vocab, NiyahSampler *s);
    float       niyah_train_step(NiyahModel *m, NiyahAdam *opt,
                                 const uint32_t *tokens, uint32_t n);
    const char *niyah_simd_name(void);   // "AVX2+FMA" | "NEON" | "Scalar"
    size_t      niyah_param_count(const NiyahModel *m);
    char *niyah_hybrid_generate(NiyahModel *m, const char *prompt,
                                const NiyahHybridOpts *opts,
                                NiyahSampler *sampler,
                                uint8_t proof_out[32]);
    OOF_STANDALONE_TEST -o test_proof && ./test_proof

# Full suite (96 tests across all subsystems)
./niyah_hybrid --smoke
