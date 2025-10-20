/* Test RISC-V Vector (RVV) support
 * Compile with: gcc -march=rv64gcv test_rvv.c -o test_rvv
 * Run: ./test_rvv
 */

#include <stdio.h>
#include <stdint.h>

#if defined(__riscv_vector)
#include <riscv_vector.h>

void test_rvv_xor(void)
{
    printf("Testing RVV intrinsics...\n");
    
    uint64_t a[2] = {0x1234567890ABCDEF, 0xFEDCBA0987654321};
    uint64_t b[2] = {0xAAAAAAAAAAAAAAAA, 0x5555555555555555};
    uint64_t result[2];
    
    // Set vector length for 2 uint64_t elements
    size_t vl = __riscv_vsetvl_e64m1(2);
    printf("Vector length: %zu\n", vl);
    
    // Load vectors
    vuint64m1_t va = __riscv_vle64_v_u64m1(a, vl);
    vuint64m1_t vb = __riscv_vle64_v_u64m1(b, vl);
    
    // XOR operation
    vuint64m1_t vr = __riscv_vxor_vv_u64m1(va, vb, vl);
    
    // Store result
    __riscv_vse64_v_u64m1(result, vr, vl);
    
    // Verify result
    uint64_t expected[2] = {
        a[0] ^ b[0],
        a[1] ^ b[1]
    };
    
    printf("Input A:    0x%016lX, 0x%016lX\n", a[0], a[1]);
    printf("Input B:    0x%016lX, 0x%016lX\n", b[0], b[1]);
    printf("RVV Result: 0x%016lX, 0x%016lX\n", result[0], result[1]);
    printf("Expected:   0x%016lX, 0x%016lX\n", expected[0], expected[1]);
    
    if (result[0] == expected[0] && result[1] == expected[1]) {
        printf("✓ RVV XOR test PASSED!\n");
    } else {
        printf("✗ RVV XOR test FAILED!\n");
    }
}

void test_rvv_add(void)
{
    printf("\nTesting RVV addition...\n");
    
    uint64_t a[2] = {100, 200};
    uint64_t b[2] = {50, 75};
    uint64_t result[2];
    
    size_t vl = __riscv_vsetvl_e64m1(2);
    vuint64m1_t va = __riscv_vle64_v_u64m1(a, vl);
    vuint64m1_t vb = __riscv_vle64_v_u64m1(b, vl);
    vuint64m1_t vr = __riscv_vadd_vv_u64m1(va, vb, vl);
    __riscv_vse64_v_u64m1(result, vr, vl);
    
    printf("A + B: %lu + %lu = %lu (expected %lu)\n", 
           a[0], b[0], result[0], a[0] + b[0]);
    printf("A + B: %lu + %lu = %lu (expected %lu)\n", 
           a[1], b[1], result[1], a[1] + b[1]);
    
    if (result[0] == (a[0] + b[0]) && result[1] == (a[1] + b[1])) {
        printf("✓ RVV ADD test PASSED!\n");
    } else {
        printf("✗ RVV ADD test FAILED!\n");
    }
}

int main(void)
{
    printf("RISC-V Vector Extension Test\n");
    printf("=============================\n\n");
    printf("__riscv_vector is defined: YES\n");
    printf("Compiler supports RVV intrinsics\n\n");
    
    test_rvv_xor();
    test_rvv_add();
    
    printf("\n✓ All RVV tests completed!\n");
    printf("\nYour system supports RISC-V Vector extensions.\n");
    printf("XMRig can use sse2rvv_optimized.h for better performance.\n");
    
    return 0;
}

#else

int main(void)
{
    printf("RISC-V Vector Extension Test\n");
    printf("=============================\n\n");
    printf("__riscv_vector is NOT defined\n");
    printf("Your compiler does not support RVV intrinsics.\n");
    printf("XMRig will use scalar fallback (sse2rvv.h).\n\n");
    printf("To enable RVV support:\n");
    printf("1. Use GCC 12+ or Clang 15+ with RVV support\n");
    printf("2. Compile with -march=rv64gcv or appropriate flags\n");
    
    return 1;
}

#endif
