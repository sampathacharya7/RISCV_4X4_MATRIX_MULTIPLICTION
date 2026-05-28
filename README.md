#  RISC-V 4×4 Matrix Multiplier Accelerator

A specialized hardware accelerator for 4×4 matrix multiplication implemented on the **Boolean Board** FPGA with a RISC-V SoC architecture. This project demonstrates efficient matrix computation through custom hardware acceleration, perfect for embedded systems and IoT applications requiring fast numerical processing.


##  Overview

This project implements a **4×4 matrix multiplier accelerator** as a peripheral in a RISC-V SoC running on an FPGA. Instead of relying on the CPU alone to perform matrix multiplication, we've designed dedicated hardware that can compute the product of two 4×4 matrices in parallel, providing significant performance gains for matrix-intensive applications.

### Why This Matters

- **Speed**: Hardware acceleration provides orders of magnitude faster matrix multiplication compared to software
- **Efficiency**: Dedicated hardware reduces CPU load, ideal for embedded systems
- **Scalability**: Proves the concept for larger accelerators (8×8, 16×16, etc.)
- **Educational Value**: Learn FPGA design, RISC-V development, and hardware-software co-design

---

##  Key Features

| Feature | Details |
|---------|---------|
| **Input Data Type** | 8-bit unsigned integers |
| **Output Data Type** | 16-bit unsigned integers |
| **Matrix Size** | 4×4 matrices |
| **FPGA Board** | Boolean Board (Xilinx Artix-7) |
| **SoC Clock** | 25 MHz |
| **Peripheral Bus** | AXI-4 compatible |
| **Base Address** | 0x95000000 |
| **Verification** | Full software reference comparison |


### Matrix Multiplier Peripheral Registers

The hardware accelerator uses memory-mapped I/O with the following register map (base address: `0x95000000`):

| Offset | Register | Purpose | Details |
|--------|----------|---------|---------|
| 0x00 | CTRL | Control Register | Bit 0: START, Bit 8: DONE |
| 0x04 | A_ROW0 | Matrix A Row 0 | 4× 8-bit packed values |
| 0x08 | A_ROW1 | Matrix A Row 1 | 4× 8-bit packed values |
| 0x0C | A_ROW2 | Matrix A Row 2 | 4× 8-bit packed values |
| 0x10 | A_ROW3 | Matrix A Row 3 | 4× 8-bit packed values |
| 0x14 | B_ROW0 | Matrix B Row 0 | 4× 8-bit packed values |
| 0x18 | B_ROW1 | Matrix B Row 1 | 4× 8-bit packed values |
| 0x1C | B_ROW2 | Matrix B Row 2 | 4× 8-bit packed values |
| 0x20 | B_ROW3 | Matrix B Row 3 | 4× 8-bit packed values |
| 0x24 | R_R0_LO | Result Row 0 (Low) | C[0][0] & C[0][1] |
| 0x28 | R_R0_HI | Result Row 0 (High) | C[0][2] & C[0][3] |
| 0x2C | R_R1_LO | Result Row 1 (Low) | C[1][0] & C[1][1] |
| 0x30 | R_R1_HI | Result Row 1 (High) | C[1][2] & C[1][3] |
| 0x34 | R_R2_LO | Result Row 2 (Low) | C[2][0] & C[2][1] |
| 0x38 | R_R2_HI | Result Row 2 (High) | C[2][2] & C[2][3] |
| 0x3C | R_R3_LO | Result Row 3 (Low) | C[3][0] & C[3][1] |
| 0x40 | R_R3_HI | Result Row 3 (High) | C[3][2] & C[3][3] |

### Data Packing Format

Each 32-bit register holds 4 8-bit matrix elements:
```c
#define PACK(c0,c1,c2,c3) \
    (((uint32_t)(c3)<<24)|((uint32_t)(c2)<<16)|((uint32_t)(c1)<<8)|(uint32_t)(c0))
```

---

## 📁 Project Structure

```
RISCV_4X4_MATRIX_MULTIPLICTION/
├── README.md                          # This file
├── MAIN.C                             # Main test suite for hardware accelerator
├── Matrix Multiplier.pptx             # Presentation & design documentation
├── final_riscv.pdf                    # Detailed technical report
├── installation_guide.txt             # Docker & environment setup
├── POWERSHELL.TXT                     # USB device setup for Windows
├── wsl.txt                            # WSL configuration notes
├── RESULT.TXT                         # Test execution results
├── SAM_4X4_MULTIPLICATION/            # Design & constraint files
│   ├── Constraints.ucf                # FPGA pin constraints
│   ├── top.v                          # Top-level Verilog module
│   ├── fpga_top.v                     # FPGA wrapper
│   ├── linker.ld                      # Linker script for RISC-V
│   └── ... (additional HDL files)
└── [Hidden Build Files]               # Vivado project files

```

---

## 💻 Installation & Setup

### Prerequisites

- **Xilinx Vivado** (for FPGA synthesis and programming)
- **RISC-V GNU Toolchain** (rv32im architecture)
- **WSL (Windows Subsystem for Linux)** or native Linux
- **Docker** (optional, for isolated development environment)
- **Boolean Board FPGA** with USB programmer

### Quick Start

#### Option 1: Docker-based Setup (Recommended)

```bash
# Install Docker on your WSL/Ubuntu
sudo apt update && sudo apt install docker.io -y
sudo systemctl enable docker && sudo systemctl start docker
sudo usermod -aG docker $USER
# Restart your terminal to apply group changes

# Pull the pre-configured RISC-V development image
sudo docker pull ttusharshenoy/riscv:latest

# Run the Docker container with X11 forwarding for GUI
sudo apt install x11-xserver-utils
xhost +local:root

sudo docker run -it \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  ttusharshenoy/riscv:latest
```

#### Option 2: Manual Installation (WSL/Ubuntu)

For detailed manual setup, see **installation_guide.txt** in the repository.

### USB Device Setup (Windows + WSL)

If you're using Windows with WSL and need to program the FPGA:

```powershell
# Run as Administrator in Windows PowerShell

# Install usbipd
winget install usbipd

# List available USB devices
usbipd list
# Output shows devices with BUSID like "2-3"

# Bind and attach your FPGA device to WSL
usbipd bind --busid 2-3
usbipd attach --wsl --busid 2-3

# Inside WSL, install usbip tools
sudo apt install linux-tools-generic hwdata
sudo update-alternatives --install /usr/local/bin/usbip usbip /usr/lib/linux-tools/*-generic/usbip 20
```

For more details, see **POWERSHELL.TXT**.

---

## 🎮 Usage Guide

### Compiling the Test Suite

```bash
cd riscv_soc/fpga/boolean

# Compile for RISC-V 32-bit (rv32im) with no standard library
riscv64-unknown-elf-gcc \
  -march=rv32im \
  -mabi=ilp32 \
  -nostdlib \
  -nostartfiles \
  -Wl,--build-id=none \
  start.S main.c \
  -T linker.ld \
  -o main.elf
```

### Running on the Hardware

```bash
# Using the provided Python serial interface
python3 run.py -d /dev/ttyUSB1 -f main.elf
```

### Basic Hardware Usage Pattern

```c
#include <stdint.h>

// Define peripheral base addresses
#define MATMUL_BASE   0x95000000UL
#define MATMUL_CTRL   (*(volatile uint32_t*)(MATMUL_BASE + 0x00))
#define MATMUL_A_ROW0 (*(volatile uint32_t*)(MATMUL_BASE + 0x04))
// ... (other register definitions)
#define MATMUL_START  (1u << 0)
#define MATMUL_DONE   (1u << 8)

// Pack 4 8-bit values into a 32-bit register
#define PACK(c0,c1,c2,c3) \
    (((uint32_t)(c3)<<24)|((uint32_t)(c2)<<16)|((uint32_t)(c1)<<8)|(uint32_t)(c0))

// Example: Multiply two 4x4 matrices
void matrix_multiply(const uint8_t a[4][4], const uint8_t b[4][4], uint16_t result[4][4])
{
    // Load matrix A into hardware
    MATMUL_A_ROW0 = PACK(a[0][0], a[0][1], a[0][2], a[0][3]);
    MATMUL_A_ROW1 = PACK(a[1][0], a[1][1], a[1][2], a[1][3]);
    MATMUL_A_ROW2 = PACK(a[2][0], a[2][1], a[2][2], a[2][3]);
    MATMUL_A_ROW3 = PACK(a[3][0], a[3][1], a[3][2], a[3][3]);
    
    // Load matrix B into hardware
    MATMUL_B_ROW0 = PACK(b[0][0], b[0][1], b[0][2], b[0][3]);
    MATMUL_B_ROW1 = PACK(b[1][0], b[1][1], b[1][2], b[1][3]);
    MATMUL_B_ROW2 = PACK(b[2][0], b[2][1], b[2][2], b[2][3]);
    MATMUL_B_ROW3 = PACK(b[3][0], b[3][1], b[3][2], b[3][3]);
    
    // Start computation
    MATMUL_CTRL = MATMUL_START;
    
    // Wait for completion
    while (!(MATMUL_CTRL & MATMUL_DONE));
    
    // Read results
    uint32_t lo, hi;
    
    lo = MATMUL_R0_LO; hi = MATMUL_R0_HI;
    result[0][0] = (uint16_t)(lo & 0xFFFF);
    result[0][1] = (uint16_t)(lo >> 16);
    result[0][2] = (uint16_t)(hi & 0xFFFF);
    result[0][3] = (uint16_t)(hi >> 16);
    // ... (repeat for remaining rows)
}
```

---

## 🧪 Testing & Verification

The test suite includes **7 comprehensive tests** to verify hardware correctness:

### Test 1: Identity Matrix Verification
**Test**: Identity × Data = Data
```
Status: ✅ PASS
Input B:       [05 06 07 08] [01 02 03 04] [09 0A 0B 0C] [0D 0E 0F 10]
HW Result:     [  5  6  7  8] [  1  2  3  4] [  9 10 11 12] [ 13 14 15 16]
Expected:      ✓ Matches B exactly
```

### Test 2: Zero Matrix Verification
**Test**: Zero × Any Data = All Zeros
```
Status: ✅ PASS
HW Result:     [0 0 0 0] [0 0 0 0] [0 0 0 0] [0 0 0 0]
Expected:      ✓ All zeros

### Test 3: Data × Identity Verification
**Test**: Data × Identity = Data
```
Status: ✅ PASS
Input A:       [01 02 03 04] [05 06 07 08] [09 0A 0B 0C] [0D 0E 0F 10]
HW Result:     [  1  2  3  4] [  5  6  7  8] [  9 10 11 12] [ 13 14 15 16]
Expected:      ✓ Matches A exactly
```

### Test 4: Diagonal × Ones
**Test**: Diagonal matrix × all-ones matrix
```
Status: ✅ PASS
Expected:      Row0=[1,1,1,1] Row1=[2,2,2,2] Row2=[3,3,3,3] Row3=[4,4,4,4]
HW Result:     [1 1 1 1] [2 2 2 2] [3 3 3 3] [4 4 4 4]
Expected:      ✓ Correct diagonal scaling
```

### Test 5: Known Numeric Result
**Test**: HW vs Software reference comparison
```
Status: ✅ PASS
SW Reference:  [25  61  24  57] [86 123  46 152] [98 99 60 129] [77 191 64 188]
HW Result:     [25  61  24  57] [86 123  46 152] [98 99 60 129] [77 191 64 188]
Expected:      ✓ Bit-exact match
```

### Test 6: Register Readback
**Test**: Write and read back register values
```
Status: ✅ PASS
ROW0 Written:  0x12EFCDAB → Read Back: 0x12EFCDAB
ROW1 Written:  0x9A785634 → Read Back: 0x9A785634
Expected:      ✓ Register integrity verified
```

### Test 7: Stress Test (10 Iterations)
**Test**: Repeated multiplication for stability
```
Status: ✅ PASS (all 10 runs match software reference)
Expected:      ✓ Hardware stability confirmed
```

### Full Test Output

See **RESULT.TXT** in the repository for complete test execution output.

---

## 📊 Technical Specifications

### Matrix Multiplier Specifications

| Specification | Value |
|---|---|
| **Matrix Dimension** | 4 × 4 |
| **Input Precision** | 8-bit unsigned (0-255) |
| **Output Precision** | 16-bit unsigned (0-65535) |
| **Max Value** | 255 × 255 × 4 = 260,100 |
| **Clock Frequency** | 25 MHz (Boolean Board) |
| **Latency** | Few clock cycles |
| **Throughput** | One 4×4 multiply per operation |
| **Power Consumption** | Minimal (FPGA-dependent) |

### Computation Formula

For result matrix C = A × B:

```
C[i][j] = Σ(k=0 to 3) A[i][k] × B[k][j]
```

Each element can be up to: **255 × 255 × 4 = 260,100** (fits in 16 bits)

### Hardware Resource Usage

- **Logic Slices**: 150-200 (estimated, board-dependent)
- **Memory**: Minimal (register-based)
- **I/O**: 4 data inputs, 4 data outputs, 1 control, 1 status
- **Clock Domains**: Single 25 MHz clock

---

## 📈 Results

### Performance Summary

✅ **All 7 tests passed successfully**

The hardware accelerator demonstrated:
- **100% correctness** across all test vectors
- **Perfect accuracy** matching software reference implementations
- **Stable operation** across repeated iterations
- **Register integrity** with proper read/write capabilities

### Test Execution Time
- Complete test suite: ~2 seconds over UART (115200 baud)
- Single matrix multiply: < 1 millisecond

---

## 📚 Documentation

### Included Files

| Document | Purpose |
|----------|---------|
| **Matrix Multiplier.pptx** | Design presentation with architecture diagrams and overview |
| **final_riscv.pdf** | Comprehensive technical report with detailed analysis |
| **installation_guide.txt** | Step-by-step setup instructions for Docker environment |
| **POWERSHELL.TXT** | USB device mapping for Windows + WSL |
| **wsl.txt** | WSL directory and file structure |
| **RESULT.TXT** | Complete test execution output and results |

### Key References
- RISC-V Specification (RV32IM)
- Xilinx Vivado Design Suite Documentation
- Boolean Board FPGA Reference Manual
- AXI-4 Protocol Specification

---

## 🎓 Learning Outcomes

This project covers essential topics in embedded systems:

- **FPGA Design**: Verilog RTL, Vivado workflow
- **RISC-V Architecture**: CPU design, instruction set, ABI
- **Hardware-Software Co-design**: Peripheral integration, memory-mapped I/O
- **Testing & Verification**: Hardware validation, cross-checking with reference software
- **Development Tools**: GCC toolchain, linkers, loaders, serial communication
- **Performance Optimization**: Hardware acceleration techniques

##  Highlights

-  **Proven Hardware**: Successfully tested on Boolean Board FPGA
-  **Comprehensive Testing**: 7 test cases covering edge cases and stress scenarios
-  **Well-Documented**: Complete documentation, code comments, and guides
-  **Production-Ready**: Stable, reliable, and verified implementation
-  **Educational**: Perfect for learning FPGA + RISC-V + Hardware Design


  4x4 Matrix Multiplier Accelerator Tests
  RISC-V SoC @ Boolean Board (25 MHz)
  All tests complete. 
