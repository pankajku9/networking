/*
 * udp-server.c
 * 
 * A simple UDP server that demonstrates an IPv6 wildcard socket binding
 * Code adaptyed from W. Stevens Unix Newtwork Programming Vol 1.
 * Compiled on Debian.
 */

#include <errno.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <time.h> 
 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h> 
 
#include <arpa/inet.h> 

#define MAXBUF 65536 

struct unp_in_pktinfo {
  struct in6_addr ipi_addr;     /* destination IPv6 address */
  int             ipi_ifindex ; /* received interface index */
  };

void 
error(char *msg) { 
  perror(msg); 
  exit(1); 
  } 


/*
 * recvfrom_flags
 *
 * receive a UDP packet and look up the destination address of the packet
 */

ssize_t
recvfrom_flags(int fd, void *ptr, size_t nbytes, int *flagsp,
	       struct sockaddr_in6 *sa, socklen_t *salenptr, struct unp_in_pktinfo *pktp)
{
  struct msghdr msg ;
  struct iovec iov[1] ;
  ssize_t n ;
  struct cmsghdr *cmptr ;
  union {
    struct cmsghdr cm;
    char control[CMSG_SPACE(sizeof(struct in6_addr)) +
                 CMSG_SPACE(sizeof(struct unp_in_pktinfo))] ;
    } control_un ;

  msg.msg_control = control_un.control ;
  msg.msg_controllen = sizeof(control_un.control) ;
  msg.msg_flags = 0 ;
  msg.msg_name = sa ;
  msg.msg_namelen = *salenptr ;
  iov[0].iov_base = ptr ;
  iov[0].iov_len = nbytes ;
  msg.msg_iov = iov ;
  msg.msg_iovlen = 1 ;

  if (((n = recvmsg(fd, &msg, *flagsp))) < 0) 
    return(n) ;

  *salenptr = msg.msg_namelen ;  /* pass back results */
  if (pktp)
    bzero(pktp, sizeof(struct unp_in_pktinfo)) ; /* set the address field and interface index to zeros */

  *flagsp = msg.msg_flags;

  if ((msg.msg_controllen < sizeof(struct cmsghdr)) ||
       (msg.msg_flags & MSG_CTRUNC) || pktp == NULL)
    return(n) ;

  /* find the packet info field and pass the destination address back */                  
  for (cmptr = CMSG_FIRSTHDR(&msg); cmptr != NULL ; 
       cmptr = CMSG_NXTHDR(&msg, cmptr)) {
    if (cmptr->cmsg_level == IPPROTO_IPV6 && cmptr->cmsg_type == IPV6_PKTINFO) {
      struct unp_in_pktinfo *p = (struct unp_in_pktinfo *) CMSG_DATA(cmptr) ;
      pktp->ipi_addr = p->ipi_addr ;
      pktp->ipi_ifindex = p->ipi_ifindex ;
      continue ;
      }

    }
  return(n) ;
}

ssize_t
sendto_flags(int fd, void *buffer, size_t bufferLen, int flags,
		struct sockaddr_in6 *clientAddr, socklen_t *clientLen,
		struct sockaddr_in6 *srvAddr, socklen_t *srvLen, uint *ifindex)
{
  struct msghdr msgheader;
  struct cmsghdr *control_msg;
  struct iovec msg_iov;
  char control_buf[256];
  struct unp_in_pktinfo *packet;
  ssize_t count;

  // Set up iov and msgheader
  memset(&msgheader, 0, sizeof(struct msghdr));
  msg_iov.iov_base = buffer;
  msg_iov.iov_len  = bufferLen;
  msgheader.msg_name = clientAddr;
  msgheader.msg_namelen = clientLen ? *clientLen : 0;
  msgheader.msg_iov  = &msg_iov;
  msgheader.msg_iovlen = 1;
  msgheader.msg_flags = 0;

  msgheader.msg_control = control_buf;
  msgheader.msg_controllen = sizeof(control_buf);

  control_msg = CMSG_FIRSTHDR(&msgheader); 
  control_msg->cmsg_level = IPPROTO_IPV6; 
  control_msg->cmsg_type = IPV6_PKTINFO; 
  control_msg->cmsg_len = CMSG_LEN(sizeof(*packet)); 

  packet = (struct unp_in_pktinfo *) CMSG_DATA(control_msg); 
  memset(packet, 0, sizeof(*packet)); 
  packet->ipi_addr = srvAddr->sin6_addr; 
  packet->ipi_ifindex = *ifindex; 
  msgheader.msg_controllen = control_msg->cmsg_len;

  // Send a packet
  if (((count = sendmsg(fd, &msgheader, flags))) < 0) 
    error("sendmsg error") ;

  return count;
}

 
int main(int argc, char *argv[]) { 
  int sockfd, portno; 
  char buffer[MAXBUF]; 
  char client_addr_ipv6[256] ;
  char dest_addr_ipv6[256] ;
  struct sockaddr_in6 serv_addr, sin6, sout6; 
  int sin6len, sout6len; 
  int status ;
  struct unp_in_pktinfo pkt ;
  int recv_pkt_info = 1 ;
  int flags = 0 ;
  uint ifindex ;
  int enable = 1 ;
 
 
  if (argc < 2) { 
    fprintf(stderr, "Usage: %s PORT_ADDRESS\n", argv[0]); 
    exit(0); 
    } 
 
  printf("\nIPv6 UDP Server Started...\n"); 
     
  //Sockets Layer Call: socket() 
  if (((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP))) < 0) 
    error("ERROR opening socket"); 
 
  if (setsockopt(sockfd,IPPROTO_IPV6,IPV6_RECVPKTINFO,&recv_pkt_info,sizeof(recv_pkt_info))<0)
    error("ERROR setting REUSE socket option") ;

  if (setsockopt(sockfd, IPPROTO_IP, IP_FREEBIND, &enable, sizeof(enable)) < 0) 
    error("ERROR setting FREEBIND socket option") ;

  bzero((char *) &serv_addr, sizeof(serv_addr)); 
 
  /* the port number is provided through the CLI as the arg to the server */ 
  portno = atoi(argv[1]); 
  serv_addr.sin6_flowinfo = 0; 
  serv_addr.sin6_family = AF_INET6; 
 
  /* the IPv6 address is the IPv6 wilrdard defined in netinet/in.h */ 
  serv_addr.sin6_addr = in6addr_any; 
  serv_addr.sin6_port = htons(portno); 
  sin6len = sizeof(sin6) ;  
 
  //Sockets Layer Call: bind()  to bind the UDP socket6 to the IPv6 :: address
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
      error("ERROR on binding"); 



  bzero((char *) &sout6, sizeof(sout6)); 
  sout6.sin6_flowinfo = 0; 
  sout6.sin6_family = AF_INET6; 
  sout6.sin6_addr = in6addr_any; 
  sout6.sin6_port = serv_addr.sin6_port;
  sout6len = sizeof(sout6) ;  


  for (;;) { 
    flags = 0 ;
    status = recvfrom_flags(sockfd, buffer, MAXBUF, &flags, &sin6, &sin6len,&pkt); 
    inet_ntop(AF_INET6, &(sin6.sin6_addr),client_addr_ipv6, 256);
    inet_ntop(AF_INET6, &(pkt.ipi_addr),dest_addr_ipv6, 256);
    printf("Incoming Packet: Src: %s Dst: %s\n",client_addr_ipv6,dest_addr_ipv6);
    buffer[status] = '\0';
    printf("Buffer : %s\n", buffer); 
    flags = 0 ;
    sout6.sin6_addr = pkt.ipi_addr ;
    ifindex = pkt.ipi_ifindex ;
    status = sendto_flags(sockfd,buffer, status, flags, &sin6, &sin6len, &sout6, &sout6len, &ifindex) ;
    }
}
