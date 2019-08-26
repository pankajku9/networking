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
 
void error(char *msg) { 
  perror(msg); 
  exit(1); 
  } 
 
#define MAXBUF 65536 
 
int main(int argc, char *argv[]) { 
  int sockfd, portno; 
  char buffer[MAXBUF]; 
  char client_addr_ipv6[256] ;
  struct sockaddr_in6 serv_addr, sin6; 
  int status ;
 
 
  if (argc < 3) { 
    fprintf(stderr, "Usage: %s IP_ADDRESS PORT_ADDRESS\n", argv[0]); 
    exit(0); 
    } 
 
  printf("\nIPv6 UDP Server Started...\n"); 
     
  bzero((char *) &serv_addr, sizeof(serv_addr)); 
 
  /* the port number is provided through the CLI as the arg to the server */ 
  portno = atoi(argv[2]); 
  serv_addr.sin6_flowinfo = 0; 
  serv_addr.sin6_family = AF_INET6; 
  inet_pton(AF_INET6, argv[1], &serv_addr.sin6_addr) ;
  serv_addr.sin6_port = htons(portno); 

  //Sockets Layer Call: socket() 
  if (((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP))) < 0) 
    error("ERROR opening socket"); 
 
  while (fgets(buffer, MAXBUF, stdin) != NULL) {

    printf("sendto(): Sending buffer: <%s>\n",buffer) ; 
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) ;

    printf("recvfrom() called\n") ; 
    status = recvfrom(sockfd, buffer, MAXBUF, 0, NULL, NULL) ;
    buffer[status] = '\0';
    printf("recvfrom() returned: <%s>\n",buffer) ;
    }
}

