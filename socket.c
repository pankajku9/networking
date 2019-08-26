
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/ether.h>
/*
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <linux/if_ether.h>
*/

#include <linux/sockios.h>
#include <linux/if_packet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <sys/stat.h>


#define __IFN    "lo"
#define PKT_SIZ  64

// "enp101s0f1" "lo"

struct socket_conf {
	int lvl;
	int id;
	int val;
	const char *name;
};

void print_pkt_info(char* str, struct ether_header *eh)
{
	printf("%s - src: %s", str, ether_ntoa((struct ether_addr*)&eh->ether_shost));
	printf(" dst: %s", ether_ntoa((struct ether_addr*)&eh->ether_dhost));
	printf(" type: 0x%x \n", ntohs(eh->ether_type));
}

int netdev_read_mac(int sockfd, const char *if_name, struct ether_addr *mac)
{
	struct ifreq ifr;
	int ret;

	strcpy(ifr.ifr_name, if_name);
	ret = ioctl(sockfd, SIOCGIFHWADDR, &ifr);
	if (ret < 0) {
		perror("ioctl failed");
		return ret;
	}
	memcpy(mac->ether_addr_octet, ifr.ifr_hwaddr.sa_data, 6);

	return 0;

}

int read_net_core(void)
{
    struct dirent *de;  // Pointer for directory entry
    char cmd[512];
    FILE *fp;
    char path[1035];

    DIR *dr = opendir("/proc/sys/net/core/");

    if (dr == NULL)
    {
        printf("Could not open current directory" );
        return 0;
    }
    printf("----------/proc/sys/net/core/------------\n");
    while ((de = readdir(dr)) != NULL){
            printf("%s : ", de->d_name);
            sprintf(cmd, "cat /proc/sys/net/core/%s", de->d_name);
           // system(cmd);

            fp = popen(cmd, "r");
	      if (fp == NULL) {
		printf("Failed to run command\n" );
		exit(1);
	      }

	      if(fgets(path, sizeof(path)-1, fp) != NULL) {
		  printf("%s", path);
		}

    }
    printf("\n");
    closedir(dr);
    return 0;
}

struct socket_conf sconf[] = {
		{SOL_SOCKET, SO_ACCEPTCONN, 0 , "SO_ACCEPTCONN"},
		{SOL_SOCKET, SO_BROADCAST, 0 , "SO_BROADCAST"},
		{SOL_SOCKET, SO_BSDCOMPAT, 0 , "SO_BSDCOMPAT"},
		{SOL_SOCKET, SO_DEBUG, 0 , "SO_DEBUG"},
		{SOL_SOCKET, SO_DOMAIN, 0 , "SO_DOMAIN"},
		{SOL_SOCKET, SO_ERROR, 0 , "SO_ERROR"},
		{SOL_SOCKET, SO_DONTROUTE, 0 , "SO_DONTROUTE"},
		{SOL_SOCKET, SO_INCOMING_CPU, 0 , "SO_INCOMING_CPU"},
		{SOL_SOCKET, SO_KEEPALIVE, 0 , "SO_KEEPALIVE"},
		{SOL_SOCKET, SO_MARK, 0 , "SO_MARK"},
		{SOL_SOCKET, SO_OOBINLINE, 0 , "SO_OOBINLINE"},
		{SOL_SOCKET, SO_SNDBUF, -1 ,"SO_SNDBUF"},
		{SOL_SOCKET, SO_RCVBUF, 0 , "SO_RCVBUF"},
		{SOL_SOCKET, SO_PRIORITY, 0 , "SO_PRIORITY"},
		{SOL_SOCKET, SO_PROTOCOL, 0 , "SO_PROTOCOL"},
		{SOL_SOCKET, SO_RCVLOWAT, 0 , "SO_RCVLOWAT"},
		{SOL_SOCKET, SO_SNDLOWAT, 0 , "SO_SNDLOWAT"},
		{SOL_SOCKET, SO_RCVTIMEO, 0 , "SO_RCVTIMEO"},
		{SOL_SOCKET, SO_SNDTIMEO, 0 , "SO_SNDTIMEO"},
		{SOL_SOCKET, SO_REUSEADDR, 0 , "SO_REUSEADDR"},
		{SOL_SOCKET, SO_REUSEPORT, 0 , "SO_REUSEPORT"},
		{SOL_SOCKET, SO_RXQ_OVFL, 0 , "SO_RXQ_OVFL"},
		{SOL_SOCKET, SO_TIMESTAMP, 0 , "SO_TIMESTAMP"},
		{SOL_SOCKET, SO_TYPE, 0 , "SO_TYPE"},
		{SOL_SOCKET, SO_BUSY_POLL, 0 , "SO_BUSY_POLL"}
};


void printf_conf(int sockfd)
{
	int len = sizeof(int);
	int val = -1;
	int test_val, read_val;
	int num_c;
	int i = 0;

	num_c = sizeof(sconf)/sizeof(struct socket_conf);

	for (i=0; i < num_c; i++)
	{
		getsockopt(sockfd, sconf[i].lvl, sconf[i].id, &sconf[i].val, &len);
		printf(" %s : %d \n", sconf[i].name, sconf[i].val);
	}

}



int sndrcv(int fd)
{
	uint8_t pkt[PKT_SIZ];
	uint8_t pkt1[PKT_SIZ];
	struct ether_header *eh;
	static const uint8_t src[6] = {0x00, 0x5d, 0x01, 0x02, 0x03, 0x04};
	static const uint8_t dst[6] = {0x00, 0x5d, 0x01, 0x02, 0x03, 0x05};
	int ret;

	memset(&pkt, 0, PKT_SIZ);
	memset(&pkt1, 0, PKT_SIZ);

	eh = (struct ether_header *)pkt;
	memcpy(eh->ether_dhost, dst, 6);
	memcpy(eh->ether_shost, src, 6);
	eh->ether_type = htons((uint16_t)ETH_P_IP);


	ret = write(fd, pkt, PKT_SIZ);
	if(ret < 0){
		perror("write error");
		return -EINVAL;
	}
	print_pkt_info("write:", eh);

	ret = read(fd, pkt1, PKT_SIZ);
	if(ret < 0){
		perror("read error");
		return -EINVAL;
	}

	eh = (struct ether_header *)pkt1;
	print_pkt_info("Recvd:", eh);

	return 0;

}

int sndrcv1(int fd)
{
	uint8_t pkt[PKT_SIZ];
	uint8_t pkt1[PKT_SIZ];
	struct ether_header *eh;
	static const uint8_t src[6] = {0x00, 0x5d, 0x01, 0x02, 0x03, 0x04};
	static const uint8_t dst[6] = {0x00, 0x5d, 0x01, 0x02, 0x03, 0x05};
	int ret;

	memset(&pkt, 0, PKT_SIZ);
	memset(&pkt1, 0, PKT_SIZ);

	eh = (struct ether_header *)pkt;
	memcpy(eh->ether_dhost, dst, 6);
	memcpy(eh->ether_shost, src, 6);
	eh->ether_type = htons((uint16_t)ETH_P_IP);


	ret = send(fd, pkt, PKT_SIZ, 0);
	if(ret < 0){
		perror("send error");
		return -EINVAL;
	}
	print_pkt_info("Sent:", eh);

	ret = recv(fd, pkt1, PKT_SIZ, 0);
	if(ret < 0){
		perror("recv error");
		return -EINVAL;
	}

	eh = (struct ether_header *)pkt1;
	print_pkt_info("Recvd:", eh);

	return 0;

}


int sndrcv2(int fd)
{
	uint8_t pkt[PKT_SIZ];
	uint8_t pkt1[PKT_SIZ];
	struct ether_header *eh;
	struct sockaddr_ll snd, rcv;
	int sll_len;
	static const uint8_t src[6] = {0x00, 0x5d, 0x01, 0x02, 0x03, 0x04};
	static const uint8_t dst[6] = {0x00, 0x5d, 0x01, 0x02, 0x03, 0x05};
	int ret;

	memset(&pkt, 0, PKT_SIZ);
	memset(&pkt1, 0, PKT_SIZ);
	memset(&snd, 0, sizeof(snd));
	memset(&rcv, 0, sizeof(rcv));


	eh = (struct ether_header *)pkt;
	memcpy(eh->ether_dhost, dst, 6);
	memcpy(eh->ether_shost, src, 6);
	eh->ether_type = htons((uint16_t)ETH_P_IP);

	snd.sll_family = AF_PACKET;
	snd.sll_protocol = ETH_P_IP;
	snd.sll_ifindex = if_nametoindex(__IFN);
	ret = sendto(fd, pkt, PKT_SIZ, 0, (struct sockaddr *)&snd, sizeof(snd));
	if(ret < 0){
		perror("sendto error");
		return -EINVAL;
	}
	print_pkt_info("Sent:", eh);


	ret = recvfrom(fd, pkt1, PKT_SIZ, 0, (struct sockaddr *)&rcv, &sll_len);
	if(ret < 0){
		perror("recvfrom error");
		//needs sudo
		return -EINVAL;
	}

	eh = (struct ether_header *)pkt1;
	print_pkt_info("Recvd:", eh);

	return 0;

}

int sndrcv3(int fd)
{
	uint8_t pkt[PKT_SIZ];
	uint8_t pkt1[PKT_SIZ];
	struct msghdr msgs, msgr;
	struct ether_header *eh;
	struct iovec pktvec, pktvec1;
	struct sockaddr_ll snd, rcv;
	int sll_len;
	static const uint8_t src[6] = {0x00, 0x5d, 0x01, 0x02, 0x03, 0x04};
	static const uint8_t dst[6] = {0x00, 0x5d, 0x01, 0x02, 0x03, 0x05};
	int ret;

	memset(&pkt, 0, PKT_SIZ);
	memset(&pkt1, 0, PKT_SIZ);

	memset(&snd, 0, sizeof(snd));
	memset(&rcv, 0, sizeof(rcv));

	memset(&msgr, 0, sizeof(msgr));
	memset(&msgs, 0, sizeof(msgs));

	memset(&pktvec, 0, sizeof(pktvec));
	memset(&pktvec1, 0, sizeof(pktvec1));


	eh = (struct ether_header *)pkt;
	memcpy(eh->ether_dhost, dst, 6);
	memcpy(eh->ether_shost, src, 6);
	eh->ether_type = htons((uint16_t)ETH_P_IP);

	snd.sll_family = AF_PACKET;
	snd.sll_protocol = ETH_P_IP;
	snd.sll_ifindex = if_nametoindex(__IFN);

	pktvec.iov_base = pkt;
	pktvec.iov_len = PKT_SIZ;

	msgs.msg_name = &snd;
	msgs.msg_namelen = sizeof(snd);

	msgs.msg_iov = &pktvec;
	msgs.msg_iovlen = 1;

	ret = sendmsg(fd, &msgs, 0);
	if(ret < 0){
		perror("read error");
		return -EINVAL;
	}
	print_pkt_info("sendmsg:", eh);

	rcv.sll_family = AF_PACKET;
	rcv.sll_protocol = ETH_P_IP;
	rcv.sll_ifindex = if_nametoindex(__IFN);
	msgr.msg_name = &rcv;
	msgr.msg_namelen = sizeof(rcv);

	pktvec1.iov_base = &pkt1;
	pktvec1.iov_len = PKT_SIZ;

	msgr.msg_iov = &pktvec1;
	msgr.msg_iovlen = 1;

	ret = recvmsg(fd, &msgr, 0);
	if(ret < 0){
		perror("recvmsg error");
		return -EINVAL;
	}

	eh = (struct ether_header *)pkt1;
	print_pkt_info("Recvd:", eh);

	return 0;

}

int main()
{
	int sockfd = -1;
	const char* if_name = __IFN;
	struct sockaddr_ll send_saddr;
	char *macs;
	struct ether_addr mac;
	int sll_len ;
	int opt;

	memset(&send_saddr, 0, sizeof(send_saddr));

	read_net_core();
	sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW ); //htons(ETH_P_ALL) ); //
	if (sockfd == -1) {
		perror("socket");
		return -1;
	}

	opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_DEBUG, &opt, sizeof(opt));
	printf_conf(sockfd);

	send_saddr.sll_family = PF_PACKET;
	send_saddr.sll_protocol = htons(ETH_P_ALL);
	send_saddr.sll_ifindex = if_nametoindex(if_name);
	if (bind(sockfd, (struct sockaddr *)&send_saddr, sizeof(send_saddr)) <
			0) {
		perror("bind failed\n");
		close(sockfd);
		return -1;
	}

	netdev_read_mac(sockfd, if_name, &mac);
	printf("So far so good: %s \n", ether_ntoa(&mac));

	sndrcv(sockfd);
	sndrcv1(sockfd);
	sndrcv2(sockfd);
	sndrcv3(sockfd);

	return 0;
}
