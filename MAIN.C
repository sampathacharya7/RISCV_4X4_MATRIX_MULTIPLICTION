//
// Compile (WSL):
//   riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 \
//     -nostdlib -nostartfiles -Wl,--build-id=none \
//     start.S counter.c -T linker.ld -o counter.elf
//
// Run:
//   python3 run.py -d /dev/ttyUSB1 -f counter.elf

#include <stdint.h>

//--------------------------------------------------------------------
// UART  (base 0x92000000)
//--------------------------------------------------------------------
#define UART_BASE     0x92000000UL
#define UART_TX       (*(volatile uint32_t*)(UART_BASE + 0x04))
#define UART_STAT     (*(volatile uint32_t*)(UART_BASE + 0x08))
#define UART_TX_FULL  0x08

void uart_putchar(char c)
{
    while (UART_STAT & UART_TX_FULL);
    UART_TX = (uint32_t)c;
}

void print(const char *s)
{
    while (*s) uart_putchar(*s++);
}

void print_nibble(uint8_t v)
{
    v &= 0xF;
    uart_putchar(v < 10 ? '0' + v : 'A' + v - 10);
}

void print_hex8(uint8_t v)
{
    print_nibble(v >> 4);
    print_nibble(v & 0xF);
}

void print_hex16(uint16_t v)
{
    print_hex8(v >> 8);
    print_hex8(v & 0xFF);
}

void print_hex32(uint32_t v)
{
    int i;
    for (i = 28; i >= 0; i -= 4)
        print_nibble((v >> i) & 0xF);
}

void print_dec(uint32_t v)
{
    char buf[6];
    int i = 5;
    buf[i] = '\0';
    if (v == 0) { uart_putchar('0'); return; }
    while (v && i > 0) { buf[--i] = '0' + (v % 10); v /= 10; }
    print(buf + i);
}

void delay(uint32_t n)
{
    volatile uint32_t i;
    for (i = 0; i < n; i++);
}

//--------------------------------------------------------------------
// Matrix Multiplier Accelerator  (base 0x95000000)
//
// Register map:
//   0x00  CTRL      bit0=START(write), bit8=DONE(read)
//   0x04  A_ROW0    8-bit x4 packed: [7:0]=col0 [15:8]=col1 [23:16]=col2 [31:24]=col3
//   0x08  A_ROW1
//   0x0C  A_ROW2
//   0x10  A_ROW3
//   0x14  B_ROW0
//   0x18  B_ROW1
//   0x1C  B_ROW2
//   0x20  B_ROW3
//   0x24  R_R0_LO   C[0][0] in [15:0], C[0][1] in [31:16]
//   0x28  R_R0_HI   C[0][2] in [15:0], C[0][3] in [31:16]
//   0x2C  R_R1_LO   C[1][0] in [15:0], C[1][1] in [31:16]
//   0x30  R_R1_HI   C[1][2] in [15:0], C[1][3] in [31:16]
//   0x34  R_R2_LO   C[2][0] in [15:0], C[2][1] in [31:16]
//   0x38  R_R2_HI   C[2][2] in [15:0], C[2][3] in [31:16]
//   0x3C  R_R3_LO   C[3][0] in [15:0], C[3][1] in [31:16]
//   0x40  R_R3_HI   C[3][2] in [15:0], C[3][3] in [31:16]
//--------------------------------------------------------------------
#define MATMUL_BASE   0x95000000UL

#define MATMUL_CTRL   (*(volatile uint32_t*)(MATMUL_BASE + 0x00))
#define MATMUL_A_ROW0 (*(volatile uint32_t*)(MATMUL_BASE + 0x04))
#define MATMUL_A_ROW1 (*(volatile uint32_t*)(MATMUL_BASE + 0x08))
#define MATMUL_A_ROW2 (*(volatile uint32_t*)(MATMUL_BASE + 0x0C))
#define MATMUL_A_ROW3 (*(volatile uint32_t*)(MATMUL_BASE + 0x10))
#define MATMUL_B_ROW0 (*(volatile uint32_t*)(MATMUL_BASE + 0x14))
#define MATMUL_B_ROW1 (*(volatile uint32_t*)(MATMUL_BASE + 0x18))
#define MATMUL_B_ROW2 (*(volatile uint32_t*)(MATMUL_BASE + 0x1C))
#define MATMUL_B_ROW3 (*(volatile uint32_t*)(MATMUL_BASE + 0x20))
#define MATMUL_R0_LO  (*(volatile uint32_t*)(MATMUL_BASE + 0x24))
#define MATMUL_R0_HI  (*(volatile uint32_t*)(MATMUL_BASE + 0x28))
#define MATMUL_R1_LO  (*(volatile uint32_t*)(MATMUL_BASE + 0x2C))
#define MATMUL_R1_HI  (*(volatile uint32_t*)(MATMUL_BASE + 0x30))
#define MATMUL_R2_LO  (*(volatile uint32_t*)(MATMUL_BASE + 0x34))
#define MATMUL_R2_HI  (*(volatile uint32_t*)(MATMUL_BASE + 0x38))
#define MATMUL_R3_LO  (*(volatile uint32_t*)(MATMUL_BASE + 0x3C))
#define MATMUL_R3_HI  (*(volatile uint32_t*)(MATMUL_BASE + 0x40))

#define MATMUL_START  (1u << 0)
#define MATMUL_DONE   (1u << 8)

// Pack 4 x 8-bit elements into one 32-bit row word
#define PACK(c0,c1,c2,c3) \
    (((uint32_t)(c3)<<24)|((uint32_t)(c2)<<16)|((uint32_t)(c1)<<8)|(uint32_t)(c0))

//--------------------------------------------------------------------
// Driver functions
//--------------------------------------------------------------------
static void load_A(const uint8_t a[4][4])
{
    MATMUL_A_ROW0 = PACK(a[0][0], a[0][1], a[0][2], a[0][3]);
    MATMUL_A_ROW1 = PACK(a[1][0], a[1][1], a[1][2], a[1][3]);
    MATMUL_A_ROW2 = PACK(a[2][0], a[2][1], a[2][2], a[2][3]);
    MATMUL_A_ROW3 = PACK(a[3][0], a[3][1], a[3][2], a[3][3]);
}

static void load_B(const uint8_t b[4][4])
{
    MATMUL_B_ROW0 = PACK(b[0][0], b[0][1], b[0][2], b[0][3]);
    MATMUL_B_ROW1 = PACK(b[1][0], b[1][1], b[1][2], b[1][3]);
    MATMUL_B_ROW2 = PACK(b[2][0], b[2][1], b[2][2], b[2][3]);
    MATMUL_B_ROW3 = PACK(b[3][0], b[3][1], b[3][2], b[3][3]);
}

static void hw_start(void)
{
    MATMUL_CTRL = MATMUL_START;
    while (!(MATMUL_CTRL & MATMUL_DONE));  // poll until done
}

static void read_result(uint16_t c[4][4])
{
    uint32_t lo, hi;

    lo = MATMUL_R0_LO; hi = MATMUL_R0_HI;
    c[0][0]=(uint16_t)(lo&0xFFFF); c[0][1]=(uint16_t)(lo>>16);
    c[0][2]=(uint16_t)(hi&0xFFFF); c[0][3]=(uint16_t)(hi>>16);

    lo = MATMUL_R1_LO; hi = MATMUL_R1_HI;
    c[1][0]=(uint16_t)(lo&0xFFFF); c[1][1]=(uint16_t)(lo>>16);
    c[1][2]=(uint16_t)(hi&0xFFFF); c[1][3]=(uint16_t)(hi>>16);

    lo = MATMUL_R2_LO; hi = MATMUL_R2_HI;
    c[2][0]=(uint16_t)(lo&0xFFFF); c[2][1]=(uint16_t)(lo>>16);
    c[2][2]=(uint16_t)(hi&0xFFFF); c[2][3]=(uint16_t)(hi>>16);

    lo = MATMUL_R3_LO; hi = MATMUL_R3_HI;
    c[3][0]=(uint16_t)(lo&0xFFFF); c[3][1]=(uint16_t)(lo>>16);
    c[3][2]=(uint16_t)(hi&0xFFFF); c[3][3]=(uint16_t)(hi>>16);
}

// Software reference multiply (runs on RISC-V CPU for verification)
static void sw_mul(const uint8_t a[4][4], const uint8_t b[4][4],
                   uint16_t c[4][4])
{
    int r, col, k;
    for (r = 0; r < 4; r++)
        for (col = 0; col < 4; col++) {
            uint32_t s = 0;
            for (k = 0; k < 4; k++) s += (uint32_t)a[r][k] * b[k][col];
            c[r][col] = (uint16_t)s;
        }
}

static int results_match(const uint16_t hw[4][4], const uint16_t sw[4][4])
{
    int r, c;
    for (r = 0; r < 4; r++)
        for (c = 0; c < 4; c++)
            if (hw[r][c] != sw[r][c]) return 0;
    return 1;
}

//--------------------------------------------------------------------
// Print helpers
//--------------------------------------------------------------------
static void print_A(const uint8_t m[4][4])
{
    int r, c;
    for (r = 0; r < 4; r++) {
        print("    [ ");
        for (c = 0; c < 4; c++) {
            print_hex8(m[r][c]);
            uart_putchar(' ');
        }
        print("]\r\n");
    }
}

static void print_result(const uint16_t m[4][4])
{
    int r, c;
    for (r = 0; r < 4; r++) {
        print("    [ ");
        for (c = 0; c < 4; c++) {
            uint16_t v = m[r][c];
            // right-pad to 5 chars
            if (v < 10000) uart_putchar(' ');
            if (v < 1000)  uart_putchar(' ');
            if (v < 100)   uart_putchar(' ');
            if (v < 10)    uart_putchar(' ');
            print_dec(v);
            uart_putchar(' ');
        }
        print("]\r\n");
    }
}

//====================================================================
// TEST 1 : Identity x Data  =>  result == Data
//====================================================================
void test1_identity(void)
{
    print("\r\n[Test 1] Identity x Data  (expect result = B)\r\n");

    uint8_t A[4][4] = {
        {1,0,0,0},
        {0,1,0,0},
        {0,0,1,0},
        {0,0,0,1}
    };
    uint8_t B[4][4] = {
        { 5, 6, 7, 8},
        { 1, 2, 3, 4},
        { 9,10,11,12},
        {13,14,15,16}
    };
    uint16_t hw[4][4], sw[4][4];

    load_A(A); load_B(B);
    hw_start();
    read_result(hw);
    sw_mul(A, B, sw);

    print("  B (expected):\r\n"); print_A(B);
    print("  HW result:\r\n");    print_result(hw);
    print(results_match(hw,sw) ? "  --> PASS\r\n" : "  --> FAIL\r\n");
}

//====================================================================
// TEST 2 : Zero matrix  =>  all zeros
//====================================================================
void test2_zero(void)
{
    print("\r\n[Test 2] Zero x Data  (expect all zeros)\r\n");

    uint8_t A[4][4] = {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};
    uint8_t B[4][4] = {
        {1,2,3,4},{5,6,7,8},{9,10,11,12},{13,14,15,16}
    };
    uint16_t hw[4][4], sw[4][4];

    load_A(A); load_B(B);
    hw_start();
    read_result(hw);
    sw_mul(A, B, sw);

    print("  HW result:\r\n"); print_result(hw);
    print(results_match(hw,sw) ? "  --> PASS\r\n" : "  --> FAIL\r\n");
}

//====================================================================
// TEST 3 : Data x Identity  =>  result == A
//====================================================================
void test3_data_x_identity(void)
{
    print("\r\n[Test 3] Data x Identity  (expect result = A)\r\n");

    uint8_t A[4][4] = {
        { 1, 2, 3, 4},
        { 5, 6, 7, 8},
        { 9,10,11,12},
        {13,14,15,16}
    };
    uint8_t B[4][4] = {
        {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}
    };
    uint16_t hw[4][4], sw[4][4];

    print("  A:\r\n"); print_A(A);

    load_A(A); load_B(B);
    hw_start();
    read_result(hw);
    sw_mul(A, B, sw);

    print("  HW result:\r\n"); print_result(hw);
    print(results_match(hw,sw) ? "  --> PASS\r\n" : "  --> FAIL\r\n");
}

//====================================================================
// TEST 4 : Diagonal x Ones
// A=diag(1,2,3,4), B=all-ones
// Row i of result = (i+1) repeated 4 times
//====================================================================
void test4_diagonal(void)
{
    print("\r\n[Test 4] Diagonal x Ones\r\n");
    print("  Expect row0=[1,1,1,1] row1=[2,2,2,2] row2=[3,3,3,3] row3=[4,4,4,4]\r\n");

    uint8_t A[4][4] = {
        {1,0,0,0},{0,2,0,0},{0,0,3,0},{0,0,0,4}
    };
    uint8_t B[4][4] = {
        {1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}
    };
    uint16_t hw[4][4], sw[4][4];

    load_A(A); load_B(B);
    hw_start();
    read_result(hw);
    sw_mul(A, B, sw);

    print("  HW result:\r\n"); print_result(hw);
    print(results_match(hw,sw) ? "  --> PASS\r\n" : "  --> FAIL\r\n");
}

//====================================================================
// TEST 5 : Known numeric result
// A = [[1,2],[3,4],...] style, compare HW vs SW exactly
//====================================================================
void test5_known(void)
{
    print("\r\n[Test 5] Known numeric result (HW vs SW reference)\r\n");

    uint8_t A[4][4] = {
        { 3, 1, 4, 1},
        { 5, 9, 2, 6},
        { 5, 3, 5, 8},
        { 9, 7, 9, 3}
    };
    uint8_t B[4][4] = {
        { 2, 7, 1, 8},
        { 2, 8, 1, 8},
        { 2, 8, 4, 5},
        { 9, 0, 4, 5}
    };
    uint16_t hw[4][4], sw[4][4];

    load_A(A); load_B(B);
    hw_start();
    read_result(hw);
    sw_mul(A, B, sw);

    print("  SW reference:\r\n"); print_result(sw);
    print("  HW result:\r\n");    print_result(hw);
    print(results_match(hw,sw) ? "  --> PASS\r\n" : "  --> FAIL\r\n");
}

//====================================================================
// TEST 6 : Register readback
// Write A rows, read them back before triggering compute
//====================================================================
void test6_readback(void)
{
    print("\r\n[Test 6] Register readback (write A rows, read back)\r\n");

    uint32_t w0 = PACK(0xAB, 0xCD, 0xEF, 0x12);
    uint32_t w1 = PACK(0x34, 0x56, 0x78, 0x9A);

    MATMUL_A_ROW0 = w0;
    MATMUL_A_ROW1 = w1;

    uint32_t r0 = MATMUL_A_ROW0;
    uint32_t r1 = MATMUL_A_ROW1;

    print("  ROW0 wrote=0x"); print_hex32(w0);
    print("  read=0x");       print_hex32(r0);
    print(r0 == w0 ? "  PASS\r\n" : "  FAIL\r\n");

    print("  ROW1 wrote=0x"); print_hex32(w1);
    print("  read=0x");       print_hex32(r1);
    print(r1 == w1 ? "  PASS\r\n" : "  FAIL\r\n");
}

//====================================================================
// TEST 7 : Stress - repeat same multiply 10 times, all must match
//====================================================================
void test7_stress(void)
{
    print("\r\n[Test 7] Stress - same multiply x10 (stability)\r\n");

    uint8_t A[4][4] = {
        { 3, 1, 4, 1},
        { 5, 9, 2, 6},
        { 5, 3, 5, 8},
        { 9, 7, 9, 3}
    };
    uint8_t B[4][4] = {
        { 2, 7, 1, 8},
        { 2, 8, 1, 8},
        { 2, 8, 4, 5},
        { 9, 0, 4, 5}
    };
    uint16_t hw[4][4], ref[4][4];
    sw_mul(A, B, ref);

    int i, all_pass = 1;
    for (i = 0; i < 10; i++) {
        load_A(A); load_B(B);
        hw_start();
        read_result(hw);
        if (!results_match(hw, ref)) { all_pass = 0; break; }
    }

    print("  Last HW result:\r\n"); print_result(hw);
    if (all_pass)
        print("  --> PASS (all 10 runs match SW)\r\n");
    else
        print("  --> FAIL\r\n");
}

//====================================================================
// main
//====================================================================
int main(void)
{
    delay(100000);

    print("================================================\r\n");
    print("  4x4 Matrix Multiplier Accelerator Tests\r\n");
    print("  RISC-V SoC @ Boolean Board (25 MHz)\r\n");
    print("  Peripheral base: 0x95000000\r\n");
    print("  Input: 8-bit unsigned  Output: 16-bit unsigned\r\n");
    print("================================================\r\n");

    test1_identity();
    test2_zero();
    test3_data_x_identity();
    test4_diagonal();
    test5_known();
    test6_readback();
    test7_stress();

    print("\r\n================================================\r\n");
    print("  All tests complete.\r\n");
    print("================================================\r\n");

    while (1);
    return 0;
}
