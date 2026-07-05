# Casper Sovereign Engine (SIMD Optimized)

Casper is a high-performance, C++17 inference engine designed for sovereign deployment of LLMs (specifically Llama-3.1-8B). It utilizes AVX2/FMA hardware acceleration to achieve near-native throughput with zero Python overhead during the inference loop.

## Technical Architecture
- **Core Engine:** Written in C++17 with raw pointer memory management.
- **Acceleration:** Hardcoded AVX2 and FMA SIMD intrinsics for optimized matrix-vector multiplication.
- **Logic Layer:** Symbolic rule enforcement via `.nrule` files (StaticJudge).
- **Interface:** Python CFFI bridge for zero-copy memory mapping between weights and the native kernel.

## Best Practices & Security
1. **Memory Alignment:** Always ensure model weights are mapped to 32-byte aligned memory addresses for optimal SIMD performance.
2. **Sovereignty:** Keep `.nrule` logic files updated to prevent model hallucination in critical mathematical domains.
3. **Deployment:** Use the provided native shared object (`libcasper.so`) to bypass Python's GIL during high-concurrency inference.
4. **Weight Quantization:** While currently FP32, future iterations should leverage INT8 quantization to reduce memory bandwidth bottlenecks.
