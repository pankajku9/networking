// From stackverflow

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "rawpacket.h"

static size_t parse_data(unsigned char *const data, const size_t size,
                         const char *const string)
{
    char *ends = strncpy((char *)data, string, size);
    return (size_t)(ends - (char *)data);
}


static int parse_hwaddr(const char *const string,
                        void *const hwaddr)
{
    unsigned int addr[6];
    char         dummy;

    if (sscanf(string, " %02x:%02x:%02x:%02x:%02x:%02x %c",
                       &addr[0], &addr[1], &addr[2],
                       &addr[3], &addr[4], &addr[5],
                       &dummy) == 6 ||
        sscanf(string, " %02x%02x%02x%02x%02x%02x %c",
                       &addr[0], &addr[1], &addr[2],
                       &addr[3], &addr[4], &addr[5],
                       &dummy) == 6) {
        if (hwaddr) {
            ((unsigned char *)hwaddr)[0] = addr[0];
            ((unsigned char *)hwaddr)[1] = addr[1];
            ((unsigned char *)hwaddr)[2] = addr[2];
            ((unsigned char *)hwaddr)[3] = addr[3];
            ((unsigned char *)hwaddr)[4] = addr[4];
            ((unsigned char *)hwaddr)[5] = addr[5];
        }
        return 0;
    }

    errno = EINVAL;
    return -1;
}

int main(int argc, char *argv[])
{
    unsigned char packet[ETH_FRAME_LEN + ETH_FCS_LEN];
    unsigned char srcaddr[6], dstaddr[6];
    int           socketfd;
    size_t        size, i;
    ssize_t       n;

    if (argc < 3 || argc > 4 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
        fprintf(stderr, "\n");
        fprintf(stderr, "Usage: %s [ -h | --help ]\n", argv[0]);
        fprintf(stderr, "       %s interface hwaddr [message]\n", argv[0]);
        fprintf(stderr, "\n");
        return 1;
    }

    if (parse_hwaddr(argv[2], &dstaddr)) {
        fprintf(stderr, "%s: Invalid destination hardware address.\n", argv[2]);
        return 1;
    }

    socketfd = rawpacket_socket(ETH_P_ALL, argv[1], &srcaddr);
    if (socketfd == -1) {
        fprintf(stderr, "%s: %s.\n", argv[1], strerror(errno));
        return 1;
    }

    memset(packet, 0, sizeof packet);

    /* Construct a QinQ header for a fake Ethernet packet type. */
    size = rawpacket_qinq(packet, sizeof packet, srcaddr, dstaddr,
                                  tci(7, 0, 1U), tci(7, 0, 2U),
                                  ETH_P_IP);
    if (!size) {
        fprintf(stderr, "Failed to construct QinQ headers: %s.\n", strerror(errno));
        close(socketfd);
        return 1;
    }

    /* Add packet payload. */
    if (argc > 3)
        size += parse_data(packet + size, sizeof packet - size, argv[3]);
    else
        size += parse_data(packet + size, sizeof packet - size, "Hello!");

    /* Pad with zeroes to minimum 64 octet length. */
    if (size < 64)
        size = 64;

    /* Send it. */
    n = send(socketfd, packet, size, 0);
    if (n == -1) {
        fprintf(stderr, "Failed to send packet: %s.\n", strerror(errno));
        shutdown(socketfd, SHUT_RDWR);
        close(socketfd);
        return 1;
    }

    fprintf(stderr, "Sent %ld bytes:", (long)n);
    for (i = 0; i < size; i++)
        fprintf(stderr, " %02x", packet[i]);
    fprintf(stderr, "\n");
    fflush(stderr);

    shutdown(socketfd, SHUT_RDWR);
    if (close(socketfd)) {
        fprintf(stderr, "Error closing socket: %s.\n", strerror(errno));
        return 1;
    }

    return 0;
}
