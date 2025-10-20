# RISC-V Optimizations for XMRig

This document describes the RISC-V specific optimizations implemented in this port of XMRig.

## Overview

XMRig has been successfully ported to RISC-V architecture with two levels of optimization:

1. **Scalar Fallback** (`sse2rvv.h`) - Pure scalar C implementations of SSE/NEON intrinsics
2. **Vector Optimized** (`sse2rvv_optimized.h`) - RVV (RISC-V Vector) intrinsics for better performance

## Hardware Requirements

### Minimum Requirements
- RISC-V 64-bit CPU (rv64gc)
- 2GB RAM minimum

### Optimal Requirements  
- RISC-V 64-bit CPU with Vector extensions (rv64gcv)
- Bit manipulation extensions (Zba, Zbb, Zbc, Zbs)
- 4GB+ RAM

### Tested Hardware
- **Orange Pi RV2** with Ky X1 microarchitecture
  - Supports: rv64imafdcv_zicond_zicsr_zifencei_zihintpause_zba_zbb_zbc_zbs_zve32f_zve32x_zve64d_zve64f_zve64x_zvl128b
  - Performance: ~33 H/s on RandomX (scalar), expected 2-3x improvement with RVV

## Build Configuration

### CMake Flags (cmake/flags.cmake)

For RISC-V builds, the following flags are automatically applied:

```cmake
# Vector extensions + bit manipulation
-march=rv64gcv_zba_zbb_zbc_zbs

# Aggressive optimizations for maximum throughput
-O3 -Ofast                  # Highest optimization level
-funroll-loops              # Loop unrolling for ILP
-fomit-frame-pointer        # Free register (limited on RISC-V)
-fno-common                 # Better code generation
-finline-functions          # Aggressive inlining
-ffast-math                 # Relaxed FP (safe for mining)
-flto                       # Link-time optimization
-minline-atomics            # Inline atomic operations
```

### Compiler Requirements

- **GCC 12+** or **Clang 15+** with RISC-V Vector support
- RISC-V toolchain with RVV intrinsics (`riscv_vector.h`)
- Optional: Binutils with RISC-V support

## Implementation Details

### SSE to RISC-V Mapping

The compatibility layer provides scalar and vectorized implementations of SSE intrinsics:

| SSE Intrinsic | Scalar Implementation | RVV Implementation |
|---------------|----------------------|-------------------|
| `_mm_xor_si128` | Scalar XOR on uint64[2] | `__riscv_vxor_vv_u64m1` |
| `_mm_add_epi64` | Scalar addition | `__riscv_vadd_vv_u64m1` |
| `_mm_and_si128` | Scalar AND | `__riscv_vand_vv_u64m1` |
| `_mm_or_si128` | Scalar OR | `__riscv_vor_vv_u64m1` |
| `_mm_slli_epi64` | Scalar shift | `__riscv_vsll_vx_u64m1` |
| `_mm_srli_epi64` | Scalar shift | `__riscv_vsrl_vx_u64m1` |
| `_mm_load_si128` | `memcpy` | `__riscv_vle64_v_u64m1` |
| `_mm_store_si128` | `memcpy` | `__riscv_vse64_v_u64m1` |

### CPU Detection

RISC-V specific CPU detection in `src/backend/cpu/platform/lscpu_riscv.cpp`:

```cpp
// Detects from /proc/cpuinfo:
// - Model name
// - ISA string (extensions)
// - Microarchitecture (uarch)
// - Vector support
// - Crypto extensions
```

### Memory Operations

RISC-V specific fence instructions:

```c
_mm_mfence()  -> fence rw,rw  (full memory barrier)
_mm_lfence()  -> fence r,r    (load barrier)
_mm_sfence()  -> fence w,w    (store barrier)
_mm_pause()   -> pause        (requires Zihintpause) or nop
```

## Performance Comparison

### Expected Performance Gains with RVV

| Algorithm | Scalar (baseline) | RVV (expected) |
|-----------|------------------|----------------|
| RandomX | 33 H/s | 70-100 H/s (2-3x) |
| CryptoNight | TBD | 2-3x improvement |
| AstroBWT | TBD | 1.5-2x improvement |

*Note: Actual performance depends on CPU implementation, memory bandwidth, and cache hierarchy.*

## Building XMRig for RISC-V

### Standard Build (Scalar)

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### Optimized Build (RVV)

To use the RVV-optimized version, replace includes in crypto files:

```bash
# Option 1: Replace sse2rvv.h with sse2rvv_optimized.h manually
# Option 2: Symlink for testing
cd src/crypto/cn
mv sse2rvv.h sse2rvv_scalar.h
ln -s sse2rvv_optimized.h sse2rvv.h
cd ../../..

# Build with optimizations
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-march=rv64gcv_zba_zbb_zbc_zbs" ..
make -j$(nproc)
```

### Debug Build

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

## Runtime Configuration

### Basic Usage

```bash
# CPU benchmark
./xmrig --bench=1M

# Pool mining
./xmrig -o pool.example.com:3333 -u YOUR_WALLET -p x
```

### RISC-V Specific Options

XMRig will automatically detect:
- CPU model from `/proc/cpuinfo`
- Available ISA extensions
- Vector capability
- Number of CPU cores

Example output:
```
 * CPU:       ky,x60 (uarch ky,x60)
 * ASSEMBLY:  auto:none
 * FEATURES:  rv64imafdcv zba zbb zbc zbs
```

## Troubleshooting

### Build Errors

**Error: `unknown cpu 'generic-rv64' for '-mtune'`**
- Solution: This has been fixed. The cmake configuration no longer uses `-mtune` for RISC-V.

**Error: `riscv_vector.h: No such file or directory`**
- Solution: Your compiler doesn't support RVV intrinsics. Use scalar version (`sse2rvv.h`).
- Install updated GCC/Clang with RVV support.

**Error: `undefined reference to __riscv_vxor_vv_u64m1`**
- Solution: Compiler supports the header but not the intrinsics. Use `-march=rv64gcv` flag.

### Runtime Crashes

**Error: `free(): invalid pointer` or `SIGABRT`**
- This was caused by improper string ownership in `lscpu_riscv.cpp`
- **Fixed** by using `const char*` cast to force String copy constructor

**Low hash rates**
- Check if vector extensions are detected: look for `FEATURES: ...v...` in output
- Verify `-march=rv64gcv` was used during build
- Check CPU frequency/governor: `cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor`

## Recent Optimizations (October 2025)

### Build-Time Improvements

1. **Enhanced Compiler Flags** (`cmake/flags.cmake`):
   - Added `-flto` (Link-Time Optimization) for cross-module inlining
   - Added `-ffast-math` for relaxed FP semantics (safe for crypto/mining)
   - Added `-finline-functions` for better function inlining
   - Added `-minline-atomics` (GCC) for atomic operation inlining
   - Optimized flag ordering for GCC and Clang

2. **New RISC-V Specific Headers**:
   - `src/crypto/riscv/riscv_crypto.h` - Crypto extensions support (Zbk*)
   - `src/crypto/riscv/riscv_memory.h` - Memory operations and barriers
   - `src/crypto/rx/RxDataset_riscv.h` - RandomX dataset optimization
   - Includes fallbacks for non-RISC-V architectures

3. **Memory Optimization** (`src/crypto/riscv/riscv_memory.h`):
   - Cache-aware prefetching for better memory throughput
   - Optimized memory barriers (acquire, release, full fence)
   - Atomic operations using RISC-V A extension
   - Cache-line aligned copy operations
   - Early-exit memory comparison

4. **Crypto Extensions Support** (`src/crypto/riscv/riscv_crypto.h`):
   - Runtime detection of Zbk* crypto extensions
   - Bit rotation operations (ROR) using Zbb if available
   - Popcount and CTZ using hardware if available
   - Fallback to software implementations
   - Infrastructure for future Zkn/Zkne/Zknh support

5. **Dataset Initialization** (`src/crypto/rx/RxDataset_riscv.h`):
   - Adaptive thread allocation (60-75% of cores)
   - CPU core affinity helper functions
   - Prefetch hints for dataset access patterns
   - Optimized cache-line aligned memory operations

### Documentation

New performance tuning guide: `doc/RISCV_PERF_TUNING.md`
- Comprehensive memory configuration (2MB and 1GB huge pages)
- Runtime optimization strategies
- Configuration examples for different use cases
- Performance diagnostics and troubleshooting
- Expected performance metrics

## Future Optimizations

### Planned Improvements (Phase 2)

1. **Zbk* Full Integration**:
   - AES-NI replacement using Zknd/Zkne when available
   - SHA-256/512 using Zknh for hashing
   - Runtime CPU feature detection with graceful fallback

2. **Advanced RVV Optimizations**:
   - VLEN-aware algorithm selection
   - Masked vector operations for gather/scatter
   - Segmented loads/stores for transposed data
   - Vector shuffle implementations

3. **NUMA Optimization** (Multi-socket RISC-V):
   - Per-socket memory affinity
   - Cross-socket bandwidth optimization
   - Nodeset-aware thread allocation

4. **Microarchitecture Tuning**:
   - Ky X1 specific optimizations
   - Cache hierarchy detection
   - Prefetch window tuning per core

5. **Profile-Guided Optimization** (PGO):
   - Collect runtime profiles
   - Rebuild with PGO for +5-10% performance

### Contributing

To add new optimizations:

1. **Profile first**:
   ```bash
   perf record -g ./xmrig --bench=1M
   perf report --stdio | head -100
   ```

2. **Identify hotspots** and add optimizations to appropriate header:
   - `riscv_memory.h` for memory operations
   - `riscv_crypto.h` for crypto/bit ops
   - `RxDataset_riscv.h` for dataset init
   - `sse2rvv_optimized.h` for vector ops

3. **Test both scalar and vector paths**:
   ```bash
   # Scalar (fallback)
   ./xmrig --bench=1M
   
   # With RVV (if enabled in build)
   # Compare performance
   ```

4. **Benchmark thoroughly**:
   ```bash
   for i in {1..5}; do
       ./xmrig --bench=10M --algo=rx/0
   done
   ```

## References

- [RISC-V Vector Extension Specification](https://github.com/riscv/riscv-v-spec)
- [RISC-V Crypto Extensions](https://github.com/riscv/riscv-crypto)
- [XMRig Documentation](https://xmrig.com/docs)
- [sse2neon Project](https://github.com/DLTcollab/sse2neon) - Inspiration for compatibility approach

## License

This RISC-V port maintains XMRig's GPL-3.0 license.

## Credits

- Original XMRig: https://github.com/xmrig/xmrig
- RISC-V port: Community contribution
- Testing: Orange Pi RV2 community

---

**Status**: Production Ready (Scalar), Experimental (RVV Optimized)
**Last Updated**: October 2025
