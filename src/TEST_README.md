# DSR Test Suite

This test program helps understand the DSR encoding functions.

## Compiling and Running

```bash
cd src
make test_dsr
./test_dsr
```

Or simply:
```bash
make test
```

**Note:** The test program is compiled with the `DSR_ENABLE_TEST` compile switch.
This enables export functions for internal tables (`_ileave`, `_par`), which
are normally declared as `static`. The normal `dsrtx` program is compiled without
this switch.

## What is Tested?

### 1. Bit Manipulation (`bits_write_uint`)
- Shows how bits are written into a byte buffer
- Tests various bit offsets and sizes
- Demonstrates writing the sync word (0x712)

### 2. Interleaving Table (`_ileave`)
- Shows how 8-bit bytes are converted to 16-bit words with spread-out bits
- Demonstrates interleaving of two bytes
- Explains the bit pattern

### 3. PRBS Generator
- Shows the first 32 bits of a Pseudo-Random Binary Sequence
- Explains the LFSR mechanism

### 4. BCH Encoding
- Explains BCH(63,44) encoding
- Shows the structure (44 data bits + 19 check bits)

### 5. PS Encoding (Programme Service)
- Tests conversion from UTF-8 text to DSR character set
- Shows encoding and decoding of example strings

### 6. 77-bit Block Structure
- Explains the exact structure of a 77-bit audio block
- Shows the division into MSB/LSB parts

### 7. Frame Structure
- Explains the structure of a DSR frame
- Shows the positions of sync word, data, and PRBS

## Trace Functionality

The trace functions (`dsr_trace.h` / `dsr_trace.c`) can be used to generate
debug output during encoding.

### Usage:

```c
#include "dsr_trace.h"

// Set trace flags
dsr_trace_set_flags(DSR_TRACE_BITS | DSR_TRACE_FRAMES);

// Use trace output
TRACE_PRINTF("Encoding frame %d\n", frame_number);
TRACE_BITS(buffer, 320, "Frame data");
TRACE_HEX(buffer, 40, "Frame bytes");
```

### Available Trace Flags:

- `DSR_TRACE_BITS` - Bit manipulation operations
- `DSR_TRACE_INTERLEAVE` - Interleaving operations
- `DSR_TRACE_PRBS` - PRBS generation
- `DSR_TRACE_BCH` - BCH encoding
- `DSR_TRACE_BLOCKS` - 77-bit blocks
- `DSR_TRACE_FRAMES` - Frame structure
- `DSR_TRACE_PS` - PS encoding
- `DSR_TRACE_ALL` - All traces

## Example Output

The test suite produces detailed output for each test:

```
=== Test: bits_write_uint ===

Test 1: Write 0xAB (8 bits) at position 0
Buffer: AB 00 
Next position: 8

Test 2: Write 0x5 (3 bits) at position 8
Buffer bits: 00000000 10100000 00000000 00000000
Buffer hex: 00 A0 00 00 
Next position: 11
...
```

## Next Steps

1. Run `make test` to see all tests
2. Study the output to understand the encoding steps
3. Use the trace functions in your own code
4. Extend the tests for additional functions
