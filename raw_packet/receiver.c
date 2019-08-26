#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include "rawpacket.h"

static volatile sig_atomic_t  done = 0;

static void handle_done(int signum)
{
    done = signum;
}

static int install_done(const int signum)
{
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_handler = handle_done;
    act.sa_flags = 0;
    if (sigaction(signum, &act, NULL))
        return errno;
    return 0;
}

static const char *protocol_name(const unsigned int protocol)
{
    static char buffer[16];
    switch (protocol & 0xFFFFU) {
    case 0x0001: return "ETH_P_802_3";
    case 0x0002: return "ETH_P_AX25";
    case 0x0003: return "ETH_P_ALL";
    case 0x0060: return "ETH_P_LOOP";
    case 0x0800: return "ETH_P_IP";
    case 0x0806: return "ETH_P_ARP";
    case 0x8100: return "ETH_P_8021Q (802.1Q VLAN)";
    case 0x88A8: return "ETH_P_8021AD (802.1AD VLAN)";
    default:
        snprintf(buffer, sizeof buffer, "0x%04x", protocol & 0xFFFFU);
        return (const char *)buffer;
    }
}

static const char *header_type(const unsigned int hatype)
{
    static char buffer[16];
    switch (hatype) {
    case   1: return "ARPHRD_ETHER: Ethernet 10Mbps";
    case   2: return "ARPHRD_EETHER: Experimental Ethernet";
    case 768: return "ARPHRD_TUNNEL: IP Tunnel";
    case 772: return "ARPHRD_LOOP: Loopback";
    default:
        snprintf(buffer, sizeof buffer, "0x%04x", hatype);
        return buffer;
    }
}

static const char *packet_type(const unsigned int pkttype)
{
    static char buffer[16];
    switch (pkttype) {
    case PACKET_HOST:      return "PACKET_HOST";
    case PACKET_BROADCAST: return "PACKET_BROADCAST";
    case PACKET_MULTICAST: return "PACKET_MULTICAST";
    case PACKET_OTHERHOST: return "PACKET_OTHERHOST";
    case PACKET_OUTGOING:  return "PACKET_OUTGOING";
    default:
        snprintf(buffer, sizeof buffer, "0x%02x", pkttype);
        return (const char *)buffer;
    }
}

static void fhex(FILE *const out,
                 const char *const before,
                 const char *const after,
                 const void *const src, const size_t len)
{
    const unsigned char *const data = src;
    size_t i;

    if (len < 1)
        return;

    if (before)
        fputs(before, out);

    for (i = 0; i < len; i++)
        fprintf(out, " %02x", data[i]);

    if (after)
        fputs(after, out);
}

int main(int argc, char *argv[])
{
    struct sockaddr_ll  addr;
    socklen_t           addrlen;
    unsigned char       data[2048];
    ssize_t             n;
    int                 socketfd, flag;

    if (argc != 2 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
        fprintf(stderr, "\n");
        fprintf(stderr, "Usage: %s [ -h | --help ]\n", argv[0]);
        fprintf(stderr, "       %s interface\n", argv[0]);
        fprintf(stderr, "\n");
        return 1;
    }

    if (install_done(SIGINT) ||
        install_done(SIGHUP) ||
        install_done(SIGTERM)) {
        fprintf(stderr, "Cannot install signal handlers: %s.\n", strerror(errno));
        return 1;
    }

    socketfd = rawpacket_socket(ETH_P_ALL, argv[1], NULL);
    if (socketfd == -1) {
        fprintf(stderr, "%s: %s.\n", argv[1], strerror(errno));
        return 1;
    }

    flag = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof flag)) {
        fprintf(stderr, "Cannot set REUSEADDR socket option: %s.\n", strerror(errno));
        close(socketfd);
        return 1;
    }

    if (setsockopt(socketfd, SOL_SOCKET, SO_BINDTODEVICE, argv[1], strlen(argv[1]) + 1)) {
        fprintf(stderr, "Cannot bind to device %s: %s.\n", argv[1], strerror(errno));
        close(socketfd);
        return 1;
    }

    while (!done) {

        memset(data, 0, sizeof data);
        memset(&addr, 0, sizeof addr);
        addrlen = sizeof addr;
        n = recvfrom(socketfd, &data, sizeof data, 0,
                     (struct sockaddr *)&addr, &addrlen);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            fprintf(stderr, "Receive error: %s.\n", strerror(errno));
            break;
        }

        printf("Received %d bytes:\n", (int)n);
        printf("\t    Protocol: %s\n", protocol_name(htons(addr.sll_protocol)));
        printf("\t   Interface: %d\n", (int)addr.sll_ifindex);
        printf("\t Header type: %s\n", header_type(addr.sll_hatype));
        printf("\t Packet type: %s\n", packet_type(addr.sll_pkttype));
        fhex(stdout, "\t     Address:", "\n", addr.sll_addr, addr.sll_halen);
        fhex(stdout, "\t        Data:", "\n", data, n);
        printf("\n");

        fflush(stdout);
    }

    shutdown(socketfd, SHUT_RDWR);
    close(socketfd);
    return 0;
}
