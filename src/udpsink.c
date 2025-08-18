// udpsink.c

#include "rf.h"


// ---- Open/Close ------------------------------------------------------------

int rf_udp_open(void **out_private, const char *host, const char *port, size_t payload_bytes)
{
    if (!out_private || !host || !port) return -1;

    struct addrinfo hints, *res = NULL, *rp = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;   // IPv4/IPv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    int err = getaddrinfo(host, port, &hints, &res);
    if (err) {
        fprintf(stderr, "getaddrinfo(%s:%s): %s\n", host, port, gai_strerror(err));
        return -1;
    }

    int sock = -1;
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock < 0) continue;
        if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(sock); sock = -1;
    }
    if (sock < 0) {
        fprintf(stderr, "UDP connect(%s:%s) fehlgeschlagen\n", host, port);
        freeaddrinfo(res);
        return -1;
    }

    // Optional: Sendepuffer größer
    int sndbuf = 1 << 20; // 1 MiB
    (void)setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));

    rf_udp_t *u = (rf_udp_t*)calloc(1, sizeof(*u));
    if (!u) { close(sock); freeaddrinfo(res); return -1; }

    u->sock = sock;
    // Standard: 1400 Bytes Payload (sicher unter MTU 1500 ohne Fragmentierung)
    u->payload = (payload_bytes && payload_bytes < 9000) ? payload_bytes : 1400;
    u->preview_done = 0;

    *out_private = u;
    freeaddrinfo(res);
    return 0;
}

int rf_udp_close(void *private)
{
    rf_udp_t *u = (rf_udp_t*)private;
    if (!u) return 0;
    if (u->sock >= 0) close(u->sock);
    free(u);
    return 0;
}