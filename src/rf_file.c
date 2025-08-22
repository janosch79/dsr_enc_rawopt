/* dsr - Digitale Satelliten Radio (DSR) encoder                         */
/*=======================================================================*/
/* Copyright 2021 Philip Heron <phil@sanslogic.co.uk>                    */
/*                                                                       */
/* This program is free software: you can redistribute it and/or modify  */
/* it under the terms of the GNU General Public License as published by  */
/* the Free Software Foundation, either version 3 of the License, or     */
/* (at your option) any later version.                                   */
/*                                                                       */
/* This program is distributed in the hope that it will be useful,       */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/* GNU General Public License for more details.                          */
/*                                                                       */
/* You should have received a copy of the GNU General Public License     */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "rf.h"


#ifndef COLOR_AMBER
#define COLOR_AMBER "\x1b[33m"
#endif
#ifndef COLOR_BLUE
#define COLOR_BLUE  "\x1b[34m"
#endif
#ifndef COLOR_RESET
#define COLOR_RESET "\x1b[0m"
#endif


#ifndef UDP_PREVIEW_BYTES
#define UDP_PREVIEW_BYTES 2048
#endif


/* File sink */
typedef struct {
	FILE *f;
	void *data;
	size_t data_size;
	int samples;
	int type;
} rf_file_t;



static int _rf_file_write_uint8(void *private, int16_t *iq_data, int samples)
{
    rf_file_t *rf = private;
    uint8_t *u8 = rf->data;
    static int once_printed = 0;

    const int MAX_BYTES_TO_SHOW = 2048;
    const int BYTES_PER_PAIR   = 2 * (int)sizeof(uint8_t);   // I + Q
    const int MAX_PAIRS_TO_SHOW = MAX_BYTES_TO_SHOW / BYTES_PER_PAIR;

    while (samples) {
        int n = (rf->samples < samples) ? rf->samples : samples;
        int i;

        // signed 16-bit -> unsigned 8-bit (Shift nach [0..255], MSB)
        for (i = 0; i < n; i++, iq_data += 2) {
            u8[2*i + 0] = (uint8_t)(((int32_t)iq_data[0] - INT16_MIN) >> 8); // I
            u8[2*i + 1] = (uint8_t)(((int32_t)iq_data[1] - INT16_MIN) >> 8); // Q
        }

        // Einmalige Vorschau (max. 2048 Bytes)
        if (!once_printed) {
            int pairs_to_show   = (n < MAX_PAIRS_TO_SHOW) ? n : MAX_PAIRS_TO_SHOW;
            int bytes_in_preview = pairs_to_show * BYTES_PER_PAIR;

            printf("Schreibe %d Bytes in Datei (uint8, Hex) – Vorschau erster Block (max. %d Bytes):\n",
                   bytes_in_preview, MAX_BYTES_TO_SHOW);

            for (int j = 0; j < pairs_to_show; ++j) {
                unsigned int I = u8[2*j + 0];
                unsigned int Q = u8[2*j + 1];
                printf("%sI:0x%02X%s %sQ:0x%02X%s   ",
                       COLOR_AMBER, I, COLOR_RESET,
                       COLOR_BLUE,  Q, COLOR_RESET);
                if (((j + 1) % 4) == 0) printf("\n");
            }
            printf("\n");
            once_printed = 1;
        }

        // Schreiben: rf->data_size sollte 2*sizeof(uint8_t) sein
        fwrite(rf->data, rf->data_size, i, rf->f);
        samples -= i;
    }

    return 0;
}


static int _rf_file_write_int8(void *private, int16_t *iq_data, int total_input_samples)
{
    rf_file_t *rf = private;
    int8_t *i8 = rf->data;
    int once_printed = 0;                // static? -> Falls die Funktion mehrfach pro Prozesslebensdauer aufgerufen wird,
    static int s_once_printed = 0;       //      lieber statisch halten:
    once_printed = s_once_printed;

    const int MAX_BYTES_TO_SHOW = 2048;
    const int BYTES_PER_PAIR = 2 * (int)sizeof(int8_t); // I+Q
    const int MAX_PAIRS_TO_SHOW = MAX_BYTES_TO_SHOW / BYTES_PER_PAIR;

    int samples = total_input_samples;

    while (samples) {
        int n = rf->samples < samples ? rf->samples : samples;  // #IQ-Paare in diesem Block
        int i;

        // Konvertieren: 16-bit -> 8-bit (MSB), interleaved I/Q
        for (i = 0; i < n; i++, iq_data += 2) {
            i8[2*i + 0] = (int8_t)(iq_data[0] >> 8);
            i8[2*i + 1] = (int8_t)(iq_data[1] >> 8);
        }

        // Einmalige Vorschau (max. 2048 Bytes)
        if (!once_printed) {
            int pairs_to_show = n < MAX_PAIRS_TO_SHOW ? n : MAX_PAIRS_TO_SHOW;
            int bytes_in_preview = pairs_to_show * BYTES_PER_PAIR;

            printf("Schreibe %d Bytes in Datei (int8) – Vorschau erster Block (max. %d Bytes):\n",
                   bytes_in_preview, MAX_BYTES_TO_SHOW);

            for (int j = 0; j < pairs_to_show; ++j) {
                uint8_t I = (uint8_t)i8[2*j + 0];
                uint8_t Q = (uint8_t)i8[2*j + 1];
                printf("%sI:0x%02X%s %sQ:0x%02X%s   ",
                       COLOR_AMBER, I, COLOR_RESET,
                       COLOR_BLUE,  Q, COLOR_RESET);
                if (((j + 1) % 4) == 0) printf("\n");
            }
            printf("\n");
            once_printed = 1;
            s_once_printed = 1;
        }

        fwrite(rf->data, rf->data_size, i, rf->f); // rf->data_size = 2 * sizeof(int8_t)
        samples -= i;
    }

    return 0;
}


static int _rf_file_write_uint16(void *private, int16_t *iq_data, int samples)
{
    rf_file_t *rf = private;
    uint16_t *u16 = rf->data;
    static int once_printed = 0;

    const int MAX_BYTES_TO_SHOW = 2048;
    const int BYTES_PER_PAIR = 2 * (int)sizeof(uint16_t); // I+Q
    const int MAX_PAIRS_TO_SHOW = MAX_BYTES_TO_SHOW / BYTES_PER_PAIR;

    while (samples) {
        int n = rf->samples < samples ? rf->samples : samples; // #IQ-Paare in diesem Block
        int i;

        // Konvertieren: signed 16-bit -> unsigned 16-bit (Offset zu [0..65535])
        for (i = 0; i < n; i++, iq_data += 2) {
            u16[2*i + 0] = (uint16_t)((int32_t)iq_data[0] - INT16_MIN);
            u16[2*i + 1] = (uint16_t)((int32_t)iq_data[1] - INT16_MIN);
        }

        if (!once_printed) {
            int pairs_to_show = n < MAX_PAIRS_TO_SHOW ? n : MAX_PAIRS_TO_SHOW;
            int bytes_in_preview = pairs_to_show * BYTES_PER_PAIR;

            printf("Schreibe %d Bytes in Datei (uint16, Hex) – Vorschau erster Block (max. %d Bytes):\n",
                   bytes_in_preview, MAX_BYTES_TO_SHOW);

            for (int j = 0; j < pairs_to_show; ++j) {
                uint16_t I = u16[2*j + 0];
                uint16_t Q = u16[2*j + 1];
                printf("%sI:0x%04X%s %sQ:0x%04X%s   ",
                       COLOR_AMBER, (unsigned)I, COLOR_RESET,
                       COLOR_BLUE,  (unsigned)Q, COLOR_RESET);
                if (((j + 1) % 4) == 0) printf("\n");
            }
            printf("\n");
            once_printed = 1;
        }

        fwrite(rf->data, rf->data_size, i, rf->f); // rf->data_size = 2 * sizeof(uint16_t)
        samples -= i;
    }

    return 0;
}


static int _rf_file_write_int16(void *private, int16_t *iq_data, int samples)
{
    rf_file_t *rf = private;
    static int once_printed = 0;

    const int MAX_BYTES_TO_SHOW = 2048;
    const int BYTES_PER_PAIR = 2 * (int)sizeof(int16_t); // I+Q
    const int MAX_PAIRS_TO_SHOW = MAX_BYTES_TO_SHOW / BYTES_PER_PAIR;

    int remaining = samples;

    while (remaining) {
        int n = rf->samples < remaining ? rf->samples : remaining;

        if (!once_printed) {
            int pairs_to_show = n < MAX_PAIRS_TO_SHOW ? n : MAX_PAIRS_TO_SHOW;
            int bytes_in_preview = pairs_to_show * BYTES_PER_PAIR;

            printf("Schreibe %d Bytes in Datei (int16, Hex) – Vorschau erster Block (max. %d Bytes):\n",
                   bytes_in_preview, MAX_BYTES_TO_SHOW);

            for (int j = 0; j < pairs_to_show; ++j) {
                uint16_t I = (uint16_t)iq_data[2*j + 0];
                uint16_t Q = (uint16_t)iq_data[2*j + 1];
                printf("%sI:0x%04X%s %sQ:0x%04X%s   ",
                       COLOR_AMBER, (unsigned)I, COLOR_RESET,
                       COLOR_BLUE,  (unsigned)Q, COLOR_RESET);
                if (((j + 1) % 4) == 0) printf("\n");
            }
            printf("\n");
            once_printed = 1;
        }

        // Direkter Write aus Eingabepuffer
        fwrite(iq_data, sizeof(int16_t) * 2, n, rf->f);

        iq_data   += 2 * n;
        remaining -= n;
    }

    return 0;
}

static int _rf_file_write_int32(void *private, int16_t *iq_data, int samples)
{
    rf_file_t *rf = private;
    int32_t *i32 = rf->data;
    static int once_printed = 0;

    const int MAX_BYTES_TO_SHOW = 2048;
    const int BYTES_PER_PAIR = 2 * (int)sizeof(int32_t); // I+Q
    const int MAX_PAIRS_TO_SHOW = MAX_BYTES_TO_SHOW / BYTES_PER_PAIR;

    while (samples) {
        int n = rf->samples < samples ? rf->samples : samples;
        int i;

        for (i = 0; i < n; i++, iq_data += 2) {
            i32[2*i + 0] = ((int32_t)iq_data[0] << 16) + (int32_t)iq_data[0];
            i32[2*i + 1] = ((int32_t)iq_data[1] << 16) + (int32_t)iq_data[1];
        }

        if (!once_printed) {
            int pairs_to_show = n < MAX_PAIRS_TO_SHOW ? n : MAX_PAIRS_TO_SHOW;
            int bytes_in_preview = pairs_to_show * BYTES_PER_PAIR;

            printf("Schreibe %d Bytes in Datei (int32, Hex) – Vorschau erster Block (max. %d Bytes):\n",
                   bytes_in_preview, MAX_BYTES_TO_SHOW);

            for (int j = 0; j < pairs_to_show; ++j) {
                uint32_t I = (uint32_t)i32[2*j + 0];
                uint32_t Q = (uint32_t)i32[2*j + 1];
                printf("%sI:0x%08X%s %sQ:0x%08X%s   ",
                       COLOR_AMBER, I, COLOR_RESET,
                       COLOR_BLUE,  Q, COLOR_RESET);
                if (((j + 1) % 4) == 0) printf("\n");
            }
            printf("\n");
            once_printed = 1;
        }

        fwrite(rf->data, rf->data_size, i, rf->f); // rf->data_size = 2 * sizeof(int32_t)
        samples -= i;
    }

    return 0;
}

static int _rf_file_write_float(void *private, int16_t *iq_data, int samples)
{
    rf_file_t *rf = private;
    float *f32 = rf->data;
    static int once_printed = 0;

    const int MAX_BYTES_TO_SHOW = 2048;
    const int BYTES_PER_PAIR = 2 * (int)sizeof(float); // I+Q
    const int MAX_PAIRS_TO_SHOW = MAX_BYTES_TO_SHOW / BYTES_PER_PAIR;
    const float scale = 1.0f / 32767.0f;

    while (samples) {
        int n = rf->samples < samples ? rf->samples : samples;
        int i;

        for (i = 0; i < n; i++, iq_data += 2) {
            f32[2*i + 0] = (float)iq_data[0] * scale;
            f32[2*i + 1] = (float)iq_data[1] * scale;
        }

        if (!once_printed) {
            int pairs_to_show = n < MAX_PAIRS_TO_SHOW ? n : MAX_PAIRS_TO_SHOW;
            int bytes_in_preview = pairs_to_show * BYTES_PER_PAIR;

            printf("Schreibe %d Bytes in Datei (float, ±1.0) – Vorschau erster Block (max. %d Bytes):\n",
                   bytes_in_preview, MAX_BYTES_TO_SHOW);

            for (int j = 0; j < pairs_to_show; ++j) {
                float I = f32[2*j + 0];
                float Q = f32[2*j + 1];
                printf("%sI:% .6f%s %sQ:% .6f%s   ",
                       COLOR_AMBER, I, COLOR_RESET,
                       COLOR_BLUE,  Q, COLOR_RESET);
                if (((j + 1) % 4) == 0) printf("\n");
            }
            printf("\n");
            once_printed = 1;
        }

        fwrite(rf->data, rf->data_size, i, rf->f); // rf->data_size = 2 * sizeof(float)
        samples -= i;
    }

    return 0;
}



static int _rf_file_write_unmod_uint8(void *private, int16_t *iq_data, int bytes)
{
    rf_file_t *rf = private;
    const uint8_t *p = (const uint8_t*)iq_data; // unmodulierte Roh-Bytes
    size_t n;

    printf("Schreibe %d unvermodulierte Roh-Bytes (Hex, ohne 0x):\n", bytes);

    size_t count = 0;          // Anzahl bereits gedruckter Bytes (für Zeilenumbrüche)
    size_t k = 0;
    size_t total = (bytes < 0) ? 0 : (size_t)bytes;

    while (k < total) {
        // Treffer: Sequenz A9 59 ab Position k?
        if (k + 1 < total && p[k] == 0xA9 && p[k + 1] == 0x59) {
            // A9 (Amber)
            printf(COLOR_AMBER "%02X" COLOR_RESET " ", p[k]);
            count++;
            if ((count % 16) == 0) printf("\n");

            // 59 (Blau)
            printf(COLOR_BLUE "%02X" COLOR_RESET " ", p[k + 1]);
            count++;
            if ((count % 16) == 0) printf("\n");

            k += 2;
            continue;
        }

        // Normale Bytes ohne Hervorhebung
        printf("%02X ", p[k]);
        count++;
        if ((count % 16) == 0) printf("\n");
        k++;
    }

    if ((count % 16) != 0) printf("\n"); // Abschlusszeile

    n = fwrite(p, 1, total, rf->f);
    return (n == total) ? 0 : -1;
}
static int _rf_file_close(void *private)
{
	rf_file_t *rf = private;
	
	if(rf->f && rf->f != stdout) fclose(rf->f);
	if(rf->data) free(rf->data);
	free(rf);
	
	return(0);
}



// ---- Writer: unmodulierte Bytes -> UDP ------------------------------------


static inline void rf_udp_refill_tokens(rf_udp_t *u)
{
    if (!u || u->bitrate_bps == 0) return;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    long long dt_ns = (long long)(now.tv_sec - u->last.tv_sec) * 1000000000LL
                    + (long long)(now.tv_nsec - u->last.tv_nsec);
    if (dt_ns <= 0) return;

    double add_bytes = ((double)u->bitrate_bps / 8.0) * ((double)dt_ns / 1e9);
    u->tokens_bytes += add_bytes;

    /* Bucket-Cap (z. B. 8x payload) */
    double cap = (double)(u->payload * 8u);
    if (u->tokens_bytes > cap) u->tokens_bytes = cap;

    u->last = now;
}



static int _rf_udp_write_unmod_uint8(void *private, int16_t *iq_data, int bytes)
{
    rf_udp_t *u = (rf_udp_t*)private;
    if (!u || u->sock < 0 || bytes <= 0) return -1;

    const uint8_t *p = (const uint8_t*)iq_data;   // unmodulierte Roh-Bytes
    size_t total = (size_t)bytes;
    size_t off = 0;

    /* einmalige Vorschau (A9 59 highlight) */
    if (!u->preview_done) {
        
        size_t show = total < UDP_PREVIEW_BYTES ? total : UDP_PREVIEW_BYTES;
        printf("UDP: Sende %zu unvermodulierte Roh-Bytes (Hex, ohne 0x):\n", show);

        size_t count = 0;
        for (size_t k = 0; k < show; ) {
            if (k + 1 < show && p[k] == 0xA9 && p[k+1] == 0x59) {
                printf(COLOR_AMBER "%02X" COLOR_RESET " ", p[k]);
                if ((++count % 16) == 0) printf("\n");
                printf(COLOR_BLUE  "%02X" COLOR_RESET " ", p[k+1]);
                if ((++count % 16) == 0) printf("\n");
                k += 2;
            } else {
                printf("%02X ", p[k]);
                if ((++count % 16) == 0) printf("\n");
                k += 1;
            }
        }
        if ((count % 16) != 0) printf("\n");
        u->preview_done = 1;
    }

    while (off < total) {
        size_t chunk = total - off;
        if (chunk > u->payload) chunk = u->payload;

        /* Pacing: warten bis genug Tokens (Bytes) vorhanden */
        if (u->bitrate_bps) {
            for (;;) {
                rf_udp_refill_tokens(u);
                if (u->tokens_bytes >= (double)chunk) break;

                double need_bytes = (double)chunk - u->tokens_bytes;
                double need_ns = (need_bytes * 8.0 * 1e9) / (double)u->bitrate_bps;
                if (need_ns < 100000.0) need_ns = 100000.0; /* min 0.1 ms */

                struct timespec ts;
                ts.tv_sec  = (time_t)(need_ns / 1e9);
                ts.tv_nsec = (long)(need_ns - (double)ts.tv_sec * 1e9);
                nanosleep(&ts, NULL);
            }
            u->tokens_bytes -= (double)chunk;
        }

        ssize_t sent = send(u->sock, p + off, chunk, 0);
        if (sent < 0) {
            // fprintf(stderr, "UDP send() failed: %s\n", strerror(errno));
            return -1;
        }
        off += (size_t)sent;
    }

    return 0;
}



// Hilfsparser für "udp://host:port", "host:port" oder "[ipv6]:port"
static int parse_udp_target(const char *spec_in, char *host, size_t host_sz, char *port, size_t port_sz)
{
    if (!spec_in || !host || !port) return -1;

    const char *s = spec_in;
    if (strncmp(s, "udp://", 6) == 0) s += 6;

    if (*s == '[') {
        // [ipv6]:port
        const char *rb = strchr(s, ']');
        if (!rb || rb[1] != ':') return -1;
        size_t hlen = (size_t)(rb - (s + 1));
        if (hlen == 0 || hlen >= host_sz) return -1;
        memcpy(host, s + 1, hlen);
        host[hlen] = '\0';

        const char *ps = rb + 2;
        if (*ps == '\0') return -1;
        strncpy(port, ps, port_sz - 1);
        port[port_sz - 1] = '\0';
        return 0;
    } else {
        // host:port (letzter ':' trennt, damit "a:b:c:5000" geht)
        const char *colon = strrchr(s, ':');
        if (!colon || colon == s || *(colon + 1) == '\0') return -1;
        size_t hlen = (size_t)(colon - s);
        if (hlen >= host_sz) return -1;
        memcpy(host, s, hlen);
        host[hlen] = '\0';

        strncpy(port, colon + 1, port_sz - 1);
        port[port_sz - 1] = '\0';
        return 0;
    }
}

int rf_file_open(rf_t *s, const char *filename, int type)
{
    // --- Spezialfall: UDP-Sink ------------------------------------------------
    if (type == RF_UNMOD_UDP) {
        if (!filename) {
            fprintf(stderr, "RF_UNMOD_UDP: Ziel fehlt (erwartet z.B. udp://127.0.0.1:5000)\n");
            return -1;
        }
        char host[256], port[32];
        if (parse_udp_target(filename, host, sizeof(host), port, sizeof(port)) != 0) {
            fprintf(stderr, "RF_UNMOD_UDP: Ziel-String ungueltig: '%s'\n", filename);
            return -1;
        }

        void *udp_priv = NULL;
        if (rf_udp_open(&udp_priv, host, port, 1400) != 0) {
            fprintf(stderr, "RF_UNMOD_UDP: Konnte UDP %s:%s nicht oeffnen.\n", host, port);
            return -1;
        }
        else{
            rf_udp_set_bitrate(udp_priv, 20480000ULL); // ~10 Mb/s
            }
           
        

        s->private = udp_priv;
        s->write   = _rf_udp_write_unmod_uint8;
        s->close   = rf_udp_close;
        return 0;
    }

    // --- Dateisink für alle anderen Typen ------------------------------------
    rf_file_t *rf = calloc(1, sizeof(rf_file_t));
    if (!rf) {
        perror("calloc");
        return -1;
    }
    rf->type = type;

    if (filename == NULL) {
        fprintf(stderr, "No output filename provided.\n");
        _rf_file_close(rf);
        return -1;
    } else if (strcmp(filename, "-") == 0) {
        rf->f = stdout;
    } else {
        rf->f = fopen(filename, "wb");
        if (!rf->f) {
            perror("fopen");
            _rf_file_close(rf);
            return -1;
        }
    }

    // Datentyp-Grundgröße (pro Sample-Wert, ohne komplex)
    switch (type)
    {
    case RF_UINT8:        rf->data_size = sizeof(uint8_t);  break;
    case RF_INT8:         rf->data_size = sizeof(int8_t);   break;
    case RF_UINT16:       rf->data_size = sizeof(uint16_t); break;
    case RF_INT16:        rf->data_size = sizeof(int16_t);  break;
    case RF_INT32:        rf->data_size = sizeof(int32_t);  break;
    case RF_FLOAT:        rf->data_size = sizeof(float);    break;
    case RF_UNMOD_UINT8:  rf->data_size = sizeof(uint8_t);  break;
    default:
        fprintf(stderr, "%s: Unrecognised data type %d\n", __func__, type);
        _rf_file_close(rf);
        return -1;
    }

    // Nur für komplexe IQ-Formate verdoppeln (I+Q). NICHT für unmodulierte Rohbytes.
    if (rf->type != RF_UNMOD_UINT8 /* && rf->type != RF_UNMOD_UDP (nicht im Dateipfad) */)
        rf->data_size *= 2;

    rf->samples = 1024;

    // Buffer nur für Typen, die nicht direkt aus Eingabe schreiben
    if (rf->type != RF_INT16) {
        rf->data = malloc(rf->data_size * rf->samples);
        if (!rf->data) {
            perror("malloc");
            _rf_file_close(rf);
            return -1;
        }
    }

    // Callback registrieren
    s->private = rf;
    s->close   = _rf_file_close;

    switch (type)
    {
    case RF_UINT8:        s->write = _rf_file_write_uint8;       break;
    case RF_INT8:         s->write = _rf_file_write_int8;        break;
    case RF_UINT16:       s->write = _rf_file_write_uint16;      break;
    case RF_INT16:        s->write = _rf_file_write_int16;       break;
    case RF_INT32:        s->write = _rf_file_write_int32;       break;
    case RF_FLOAT:        s->write = _rf_file_write_float;       break;
    case RF_UNMOD_UINT8:  s->write = _rf_file_write_unmod_uint8; break;
    default:
        fprintf(stderr, "rf_file_open: Unrecognised data type %d\n", rf->type);
        _rf_file_close(rf);
        return -1;
    }

    return 0;
}

