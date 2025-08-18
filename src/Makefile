
CC      := gcc
PKGCONF := pkg-config
CFLAGS  := -g -Wall -O3 -pthread
LDFLAGS := -g -lm -pthread
OBJS    := dsrtx.o dsr.o conf.o src.o src_tone.o src_rawaudio.o rf.o rf_file.o rf_hackrf.o udpsink.o
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
	rm -f *.o *.d dsrtx

-include $(OBJS:.o=.d)



ab_dump: ab_dump.c
	$(CC) $(CFLAGS) -std=c11 -Wall -Wextra -Wpedantic -o $@ $<

.PHONY: ab_dump


ab_dump_interleaved: ab_dump_interleaved.c
	$(CC) $(CFLAGS) -std=c11 -Wall -Wextra -Wpedantic -o $@ $<
.PHONY: ab_dump_interleaved
