
CC      := gcc
PKGCONF := pkg-config
CFLAGS  := -g -Wall -O3 -pthread
LDFLAGS := -g -lm -pthread
OBJS    := dsrtx.o dsr.o bits.o conf.o src.o src_tone.o src_rawaudio.o rf.o rf_file.o rf_hackrf.o udpsink.o
PKGS    := libhackrf

FFMPEG := $(shell $(PKGCONF) --exists libavcodec && echo ffmpeg)
ifeq ($(FFMPEG),ffmpeg)
	OBJS += src_ffmpeg.o
	PKGS += libavcodec libavformat libavdevice libswresample libavutil
	CFLAGS += -DHAVE_FFMPEG
endif


CFLAGS  += $(shell $(PKGCONF) --cflags $(PKGS))
LDFLAGS += $(shell $(PKGCONF) --libs $(PKGS))

all: dsrtx

dsrtx: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) $(CFLAGS) -MM $< -o $(@:.o=.d)

clean:
	rm -f *.o *.d dsrtx test_dsr test_modulation dsr_test.d

-include $(OBJS:.o=.d)



ab_dump: ab_dump.c
	$(CC) $(CFLAGS) -std=c11 -Wall -Wextra -Wpedantic -o $@ $<

.PHONY: ab_dump


ab_dump_interleaved: ab_dump_interleaved.c
	$(CC) $(CFLAGS) -std=c11 -Wall -Wextra -Wpedantic -o $@ $<
.PHONY: ab_dump_interleaved

# Test-specific object files (compiled with DSR_ENABLE_TEST)
dsr_test.o: dsr.c Makefile
	$(CC) $(CFLAGS) -DDSR_ENABLE_TEST -c $< -o $@
	@$(CC) $(CFLAGS) -DDSR_ENABLE_TEST -MM $< -o dsr_test.d

test_dsr.o: test_dsr.c
	$(CC) $(CFLAGS) -DDSR_ENABLE_TEST -c $< -o $@

dsr_trace.o: dsr_trace.c
	$(CC) $(CFLAGS) -DDSR_ENABLE_TEST -c $< -o $@

test_dsr: test_dsr.o dsr_test.o bits.o dsr_trace.o
	$(CC) $(CFLAGS) -DDSR_ENABLE_TEST -o $@ test_dsr.o dsr_test.o bits.o dsr_trace.o $(LDFLAGS)

.PHONY: test
test: test_dsr
	./test_dsr

test_modulation: test_modulation.o dsr.o bits.o rf.o rf_file.o udpsink.o
	$(CC) $(CFLAGS) -o $@ test_modulation.o dsr.o bits.o rf.o rf_file.o udpsink.o $(LDFLAGS)

test_modulation.o: test_modulation.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: test-modulation
test-modulation: test_modulation
	./test_modulation

clean: clean-test

clean-test:
	rm -f test_modulation test_modulation.o
