# DSR Modulation Format Test Suite

This test program generates fixed random test data for all 32 channels and outputs it in all available modulation formats.

## Compiling and Running

```bash
cd src
make test-modulation
./test_modulation
```

Or simply:
```bash
make test-modulation
```

## What is Tested?

The program generates **100ms** of test data (50 blocks of 2ms each) with:

- **32 channels** with fixed random data (reproducible via seed `0x12345678`)
- **64 samples per channel per block** (32 kHz sample rate)
- **All available modulation formats**

### Tested Formats:

1. **uint8** (modulated) - 8-bit unsigned integer, QPSK-modulated
2. **int8** (modulated) - 8-bit signed integer, QPSK-modulated
3. **uint16** (modulated) - 16-bit unsigned integer, QPSK-modulated
4. **int16** (modulated) - 16-bit signed integer, QPSK-modulated
5. **int32** (modulated) - 32-bit signed integer, QPSK-modulated
6. **float** (modulated) - 32-bit float, QPSK-modulated
7. **unmod_uint8** (raw) - Unmodulated raw bytes directly from DSR encoder

**Note:** `unmod_udp` is not tested as it requires a UDP socket connection.

## Output Files

All test files are saved in the `test_output/` directory:

- `test_output/test_uint8_modulated.iq`
- `test_output/test_int8_modulated.iq`
- `test_output/test_uint16_modulated.iq`
- `test_output/test_int16_modulated.iq`
- `test_output/test_int32_modulated.iq`
- `test_output/test_float_modulated.iq`
- `test_output/test_unmod_uint8_raw.bin`

## Test Parameters

- **Duration:** 100ms
- **Blocks:** 50 (2ms per block)
- **Channels:** 32
- **Samples per channel per block:** 64
- **Random Seed:** 0x12345678 (for reproducible data)

## Using the Test Data

The generated files can be used to:

1. **Test format conversions** - Compare the different formats
2. **Test decoders** - Use the files as input for DSR decoders
3. **Signal analysis** - Analyze the modulated signals with tools like GNU Radio, SDR#, etc.
4. **Quality checks** - Compare signal quality between different formats

## Example Output

```
========================================
DSR Modulation Format Test Suite
========================================

This program generates fixed random test data for all 32 channels
and outputs it in all available modulation formats.

Test parameters:
  - Duration: 0.100 seconds (100ms)
  - Blocks: 50 (2ms per block)
  - Channels: 32
  - Samples per channel per block: 64
  - Random seed: 0x12345678

Output directory: test_output/

IMPORTANT: ALL formats use the SAME audio data and SAME DSR encoder.
           This guarantees 1:1 identical content basis for all files.
           The unmod_uint8 file contains the raw DSR bits.
           Modulated formats contain QPSK-modulated IQ samples.

Generating audio data for 50 blocks...
✓ Audio data generated (reproducible seed: 0x12345678)

========================================
Testing all modulation formats...
========================================

=== Testing format: uint8 (modulated) ===
Output file: test_output/test_uint8_modulated.iq
Generating 50 blocks (0.100 seconds)...
  Progress: 10/50 blocks (20.0%)
  Progress: 20/50 blocks (40.0%)
  ...
✓ Completed: uint8 (modulated)

...

========================================
✓ All tests completed successfully!

Generated files:
  test_output/test_uint8_modulated.iq
  test_output/test_int8_modulated.iq
  ...
========================================
```

## Technical Details

### Modulated Formats

The modulated formats use **QPSK (Quadrature Phase Shift Keying)** modulation:
- Sample rate: 20.48 MHz (2 × DSR_SYMBOL_RATE)
- Interpolation: 2
- Root-Raised-Cosine filter with rolloff factor 0.5
- Hamming window for spectrum shaping

### Unmodulated Format

The `unmod_uint8` format outputs the **raw DSR bits** directly:
- 40960 bits per block = 5120 bytes
- No modulation, no QPSK encoding
- Direct output from `dsr_encode()`

## Troubleshooting

If the program doesn't compile:
- Make sure all dependencies are installed
- Check if `rf.h` and `rf_file.h` are available
- On Windows: `unistd.h` may not be available (Linux-specific)

## Next Steps

1. Run `make test-modulation`
2. Check the generated files in `test_output/`
3. Use the files for your tests
4. Compare the different formats
