#include <sys/socket.h>
#include <linux/netlink.h>
#include <string.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <stdio.h>
#include <unistd.h>

#include "../netlink/utils.h"

#define NLINK_MSG_LEN 1024
#define DST_NL_PID  7001
#define SRC_NL_PID  7002

#define CMDARGS_EN 1

void usage(void){
	printf("-p : send port numbere\n");
	printf("-c : count of packet\n");
	printf("-s : run as server \n");
	printf("./ncli -s 1 -p 5000 -c 10");
	exit(0);
}

int send_main (long dport, long count);
int recv_main (long port, long count);

int main(int argc, char** argv)
{
	char opt;
	int nopt = 0;
	long count = 1;
	long port = DST_NL_PID;
	long srv = 0;

#if(CMDARGS_EN)
	while (nopt++, (opt = getopt(argc, argv, "c:p:s:")) != -1) switch (opt) {
		case 'p' :
			port = strtol(optarg, NULL, 10);
			break;
		case 'c' :
			count = strtol(optarg, NULL, 10);
			break;
		case 's' :
			srv = strtol(optarg, NULL, 10);
			break;
		case '?' :
			printf("error: unknown param\n");
			usage();
			break;
		default:
			usage();
	}
	/* No option in cmd args*/
	if (nopt == 1 && argc > 1)
		usage();
#endif
	if (srv)
		return recv_main(port, count);

	return send_main(port, count);
}

int recv_main (long sport, long count)
{
	struct nlmsghdr *nlh;
	struct sockaddr_nl src_addr;
	struct msghdr msg;
	struct sockaddr_nl dest_addr;
	struct iovec iov;
	int fd, ret = 0;
	char buf[128];

	memset(&src_addr, 0, sizeof(struct sockaddr_nl));
	memset(&dest_addr, 0, sizeof(struct sockaddr_nl));
	memset(&msg, 0 , sizeof(struct msghdr));
	memset(&iov, 0 , sizeof(struct iovec));
	nlh = calloc(NLMSG_SPACE(NLINK_MSG_LEN), 1);
	ret_on_null(nlh, "nlh_alloc");

	fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
	goto_on_err(free_nlh_, fd, "socket_call");

	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = sport;
	src_addr.nl_groups = 0;
	ret = bind(fd, (struct sockaddr *)&src_addr, sizeof(src_addr));
	goto_on_err(close_fd_, ret, "sock_bind");


	//dest_addr.nl_family = AF_NETLINK;
	//dest_addr.nl_pid = DST_NL_PID;
	//dest_addr.nl_groups = 0;
	msg.msg_name = (void *)&dest_addr;
	msg.msg_namelen = sizeof(dest_addr);


	iov.iov_base = (void *)nlh;
	iov.iov_len = NLMSG_SPACE(NLINK_MSG_LEN);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	sprintf(buf, "Listening  on %ld \n", sport);
	write(1, buf, strlen(buf));
	do {
		ret = recvmsg(fd, &msg, 0);
		if(ret < 0 ){
			perror("recvmsg");
			goto close_fd_;
		}
		memcpy(nlh, msg.msg_iov, msg.msg_iovlen);
		printf("Recived:%s\n", (char*)NLMSG_DATA(nlh));
	} while(--count);

close_fd_:
	close(fd);
free_nlh_:
	free(nlh);
	return 0;
}

int send_main (long dport, long count)
{
	int rv = -1;
	int fd;
	struct sockaddr_nl src_addr;
	struct sockaddr_nl dest_addr;
	struct msghdr msg;
	struct nlmsghdr *nlh;
	struct iovec iov;

	memset(&msg, 0 , sizeof(msg));
	memset(&iov, 0 , sizeof(iov));
	memset(&dest_addr, 0, sizeof(struct sockaddr_nl));
	memset(&src_addr, 0, sizeof(struct sockaddr_nl));
	nlh = calloc(NLMSG_SPACE(NLINK_MSG_LEN), 1);
	ret_on_null(rv, "nlh alloc failed");

	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
	goto_on_err(nlh_free_, fd, "socket call failed");

#if(LISTEN_EN)
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid();
	src_addr.nl_groups = 0;
	rv = bind(fd, (struct sockaddr *)&src_addr, sizeof(src_addr));
	goto_on_err(close_fd_, rv, "socket_bind_failed");
#endif

	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = dport;
	dest_addr.nl_groups = 0;
	msg.msg_name = &dest_addr;
	msg.msg_namelen = sizeof(dest_addr);

	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_flags = 0;
	nlh->nlmsg_len = NLMSG_SPACE(NLINK_MSG_LEN);

	iov.iov_base = nlh;
	iov.iov_len = nlh->nlmsg_len;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	printf("Sending %ld msg on netlink socket %ld \n", count, dport);
	do {
		sprintf(NLMSG_DATA(nlh), "Hello Process%ld", count);
		rv = sendmsg(fd, &msg, 0);
		if(rv < 0) {
			perror("sendmsg failed");
			goto close_fd_;
		}
		sleep(1);
	} while(--count);

close_fd_:
	close(fd);
nlh_free_:
	free(nlh);
	return rv;

}
