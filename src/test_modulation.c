/* dsr - Test program for all modulation formats                          */
/*=======================================================================*/
/* This program generates fixed random test data for all 32 channels     */
/* and outputs it in all available modulation formats for 100ms.        */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "dsr.h"
#include "rf.h"
#include "rf_file.h"

/* Fixed seed for reproducible test data */
#define TEST_SEED 0x12345678

/* Test duration: 100ms */
#define TEST_DURATION_SECONDS 0.1
#define DSR_BLOCK_RATE 500  /* 500 blocks per second (2ms per block) */
#define TEST_BLOCKS ((int)(TEST_DURATION_SECONDS * DSR_BLOCK_RATE))

/* Generate fixed random test data for all 32 channels */
static void generate_test_audio(int16_t *audio, int block_num)
{
	int i, ch;
	uint32_t seed = TEST_SEED + block_num;
	
	/* Generate pseudo-random but reproducible data */
	for(ch = 0; ch < 32; ch++) {
		for(i = 0; i < 64; i++) {
			/* Simple LCG for pseudo-random generation */
			seed = seed * 1103515245 + 12345;
			/* Generate values in range -32768 to 32767 */
			audio[ch * 64 + i] = (int16_t)(seed >> 16);
		}
	}
}

/* Test a specific modulation format */
static int test_modulation_format(int data_type, const char *format_name, const char *filename, int16_t (*audio_data)[TEST_BLOCKS][64 * 32])
{
	dsr_t dsr;
	rf_t rf;
	rf_qpsk_t qpsk;
	uint8_t block[5120];
	int16_t modulated[40960 * 2 * 5];
	int block_num;
	int l;
	
	printf("\n=== Testing format: %s ===\n", format_name);
	printf("Output file: %s\n", filename);
	
	/* Initialize DSR encoder - FRESH instance for each format */
	/* This ensures identical encoding state for same audio input */
	dsr_init(&dsr);
	
	/* Open output file */
	if(rf_file_open(&rf, filename, data_type) != 0) {
		fprintf(stderr, "ERROR: Failed to open output file: %s\n", filename);
		return -1;
	}
	
	/* Initialize QPSK modulator (only needed for modulated formats) */
	if(data_type != RF_UNMOD_UINT8 && data_type != RF_UNMOD_UDP) {
		/* Use sample rate that's a multiple of DSR_SYMBOL_RATE */
		unsigned int sample_rate = DSR_SYMBOL_RATE * 2;
		rf_qpsk_init(&qpsk, sample_rate / DSR_SYMBOL_RATE, 0.8 * rf_scale(&rf));
	}
	
	printf("Generating %d blocks (%.3f seconds)...\n", TEST_BLOCKS, TEST_DURATION_SECONDS);
	
	/* Generate and encode test data */
	/* IMPORTANT: ALL formats use IDENTICAL audio data and FRESH encoder */
	/*            This guarantees 1:1 identical DSR-encoded bits for each block */
	/*            across all formats (before modulation/format conversion) */
	for(block_num = 0; block_num < TEST_BLOCKS; block_num++) {
		/* Use pre-generated audio data - IDENTICAL for all formats */
		/* Encode DSR block - produces IDENTICAL bits for all formats */
		dsr_encode(&dsr, block, (*audio_data)[block_num]);
		
		/* Write output in requested format */
		if(data_type == RF_UNMOD_UINT8) {
			/* Unmodulated: write raw bytes directly */
			rf_write(&rf, (int16_t*)block, 5120); /* 40960 bits / 8 = 5120 bytes */
		} else {
			/* Modulated: QPSK modulate then write */
			l = rf_qpsk_modulate(&qpsk, modulated, block, 40960);
			rf_write(&rf, modulated, l);
		}
		
		/* Progress indicator */
		if((block_num + 1) % 10 == 0 || block_num + 1 == TEST_BLOCKS) {
			printf("  Progress: %d/%d blocks (%.1f%%)\n", 
				block_num + 1, TEST_BLOCKS, 
				100.0 * (block_num + 1) / TEST_BLOCKS);
		}
	}
	
	/* Cleanup */
	if(data_type != RF_UNMOD_UINT8 && data_type != RF_UNMOD_UDP) {
		rf_qpsk_free(&qpsk);
	}
	rf_close(&rf);
	
	printf("✓ Completed: %s\n", format_name);
	return 0;
}

int main(int argc, char **argv)
{
	const char *output_dir = "test_output";
	int errors = 0;
	dsr_t dsr;
	int16_t audio_data[TEST_BLOCKS][64 * 32];
	int block_num;
	
	printf("========================================\n");
	printf("DSR Modulation Format Test Suite\n");
	printf("========================================\n");
	printf("\nThis program generates fixed random test data for all 32 channels\n");
	printf("and outputs it in all available modulation formats.\n");
	printf("\nTest parameters:\n");
	printf("  - Duration: %.3f seconds (100ms)\n", TEST_DURATION_SECONDS);
	printf("  - Blocks: %d (2ms per block)\n", TEST_BLOCKS);
	printf("  - Channels: 32\n");
	printf("  - Samples per channel per block: 64\n");
	printf("  - Random seed: 0x%08X\n", TEST_SEED);
	printf("\nOutput directory: %s/\n", output_dir);
	printf("\nIMPORTANT: ALL formats use the SAME audio data and SAME DSR encoder.\n");
	printf("           This guarantees 1:1 identical content basis for all files.\n");
	printf("           The unmod_uint8 file contains the raw DSR bits.\n");
	printf("           Modulated formats contain QPSK-modulated IQ samples.\n");
	
	/* Create output directory if it doesn't exist */
	char cmd[256];
	snprintf(cmd, sizeof(cmd), "mkdir -p %s", output_dir);
	(void)system(cmd);
	
	/* Generate ALL audio data ONCE - shared across all formats */
	/* This ensures IDENTICAL audio input for all formats */
	printf("\nGenerating audio data for %d blocks...\n", TEST_BLOCKS);
	for(block_num = 0; block_num < TEST_BLOCKS; block_num++) {
		generate_test_audio(audio_data[block_num], block_num);
	}
	printf("✓ Audio data generated (reproducible seed: 0x%08X)\n", TEST_SEED);
	
	/* Test all modulation formats */
	printf("\n========================================\n");
	printf("Testing all modulation formats...\n");
	printf("========================================\n");
	printf("\nNOTE: ALL formats use IDENTICAL audio data.\n");
	printf("      Each format uses a FRESH DSR encoder (same initialization).\n");
	printf("      This ensures 1:1 identical DSR-encoded bits for each block.\n");
	printf("      The unmod_uint8 file contains the raw DSR bits (reference).\n");
	printf("      Modulated formats contain QPSK-modulated IQ samples.\n\n");
	
	/* Modulated formats - all use identical audio data and fresh encoder */
	if(test_modulation_format(RF_UINT8, "uint8 (modulated)", 
		"test_output/test_uint8_modulated.iq", &audio_data) != 0) errors++;
	
	if(test_modulation_format(RF_INT8, "int8 (modulated)", 
		"test_output/test_int8_modulated.iq", &audio_data) != 0) errors++;
	
	if(test_modulation_format(RF_UINT16, "uint16 (modulated)", 
		"test_output/test_uint16_modulated.iq", &audio_data) != 0) errors++;
	
	if(test_modulation_format(RF_INT16, "int16 (modulated)", 
		"test_output/test_int16_modulated.iq", &audio_data) != 0) errors++;
	
	if(test_modulation_format(RF_INT32, "int32 (modulated)", 
		"test_output/test_int32_modulated.iq", &audio_data) != 0) errors++;
	
	if(test_modulation_format(RF_FLOAT, "float (modulated)", 
		"test_output/test_float_modulated.iq", &audio_data) != 0) errors++;
	
	/* Unmodulated format - contains the raw DSR bits (reference) */
	if(test_modulation_format(RF_UNMOD_UINT8, "unmod_uint8 (raw)", 
		"test_output/test_unmod_uint8_raw.bin", &audio_data) != 0) errors++;
	
	printf("\n========================================\n");
	if(errors == 0) {
		printf("✓ All tests completed successfully!\n");
		printf("\nGenerated files:\n");
		printf("  test_output/test_uint8_modulated.iq\n");
		printf("  test_output/test_int8_modulated.iq\n");
		printf("  test_output/test_uint16_modulated.iq\n");
		printf("  test_output/test_int16_modulated.iq\n");
		printf("  test_output/test_int32_modulated.iq\n");
		printf("  test_output/test_float_modulated.iq\n");
		printf("  test_output/test_unmod_uint8_raw.bin\n");
		printf("\nNote: unmod_udp format requires UDP socket setup\n");
		printf("      and is not tested in this standalone test.\n");
	} else {
		printf("✗ %d test(s) failed!\n", errors);
	}
	printf("========================================\n");
	
	return (errors > 0) ? 1 : 0;
}

