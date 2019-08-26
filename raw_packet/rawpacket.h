#ifndef   RAWPACKET_H
#define   RAWPACKET_H
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

static int rawpacket_socket(const int protocol,
                            const char *const interface,
                            void *const hwaddr)
{
    struct ifreq        iface;
    struct sockaddr_ll  addr;
    int                 socketfd, result;
    int                 ifindex = 0;

    if (!interface || !*interface) {
        errno = EINVAL;
        return -1;
    }

    socketfd = socket(AF_PACKET, SOCK_RAW, htons(protocol));
    if (socketfd == -1)
        return -1;

    do {

        memset(&iface, 0, sizeof iface);
        strncpy((char *)&iface.ifr_name, interface, IFNAMSIZ);
        result = ioctl(socketfd, SIOCGIFINDEX, &iface);
        if (result == -1)
            break;
        ifindex = iface.ifr_ifindex;

        memset(&iface, 0, sizeof iface);
        strncpy((char *)&iface.ifr_name, interface, IFNAMSIZ);
        result = ioctl(socketfd, SIOCGIFFLAGS, &iface);
        if (result == -1)
            break;
        iface.ifr_flags |= IFF_PROMISC;
        result = ioctl(socketfd, SIOCSIFFLAGS, &iface);
        if (result == -1)
            break;

        memset(&iface, 0, sizeof iface);
        strncpy((char *)&iface.ifr_name, interface, IFNAMSIZ);
        result = ioctl(socketfd, SIOCGIFHWADDR, &iface);
        if (result == -1)
            break;

        memset(&addr, 0, sizeof addr);
        addr.sll_family = AF_PACKET;
        addr.sll_protocol = htons(protocol);
        addr.sll_ifindex = ifindex;
        addr.sll_hatype = 0;
        addr.sll_pkttype = 0;
        addr.sll_halen = ETH_ALEN; /* Assume ethernet! */
        memcpy(&addr.sll_addr, &iface.ifr_hwaddr.sa_data, addr.sll_halen);
        if (hwaddr)
            memcpy(hwaddr, &iface.ifr_hwaddr.sa_data, ETH_ALEN);

        if (bind(socketfd, (struct sockaddr *)&addr, sizeof addr))
            break;

        errno = 0;
        return socketfd;

    } while (0);

    {
        const int saved_errno = errno;
        close(socketfd);
        errno = saved_errno;
        return -1;
    }
}

static unsigned int tci(const unsigned int priority,
                        const unsigned int drop,
                        const unsigned int vlan)
{
    return (vlan & 0xFFFU)
         | ((!!drop) << 12U)
         | ((priority & 7U) << 13U);
}

static size_t rawpacket_qinq(unsigned char *const buffer, size_t const length,
                             const unsigned char *const srcaddr,
                             const unsigned char *const dstaddr,
                             const unsigned int service_tci,
                             const unsigned int customer_tci,
                             const unsigned int ethertype)
{
    unsigned char *ptr = buffer;
    uint32_t       tag;
    uint16_t       type;

    if (length < 2 * ETH_ALEN + 4 + 4 + 2) {
        errno = ENOSPC;
        return (size_t)0;
    }

    memcpy(ptr, dstaddr, ETH_ALEN);
    ptr += ETH_ALEN;

    memcpy(ptr, srcaddr, ETH_ALEN);
    ptr += ETH_ALEN;

    /* Service 802.1AD tag. */
    tag = htonl( ((uint32_t)(ETH_P_8021AD) << 16U)
               | ((uint32_t)service_tci & 0xFFFFU) );
    memcpy(ptr, &tag, 4);
    ptr += 4;

    /* Client 802.1Q tag. */
    tag = htonl( ((uint32_t)(ETH_P_8021Q) << 16U)
               | ((uint32_t)customer_tci & 0xFFFFU) );
    memcpy(ptr, &tag, 4);
    ptr += 4;

    /* Ethertype tag. */
    type = htons((uint16_t)ethertype);
    memcpy(ptr, &type, 2);
    ptr += 2;

    return (size_t)(ptr - buffer);
}

#endif /* RAWPACKET_H */
