#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/socket.h>
#include <linux/netlink.h>
#include <netlink/socket.h>
#include <netlink/netlink.h>
#include <netlink/msg.h>
#include "../netlink/utils.h"

//TODO add message construction and attribute parsing

#define NLINK_MSG_LEN 1024
#define DST_NL_PID  7001
#define SRC_NL_PID  7002

#define CMDARGS_EN 1

void usage(void) {
	printf("-p : send port numbere\n");
	printf("-c : count of packet\n");
	printf("-s : run as server \n");
	printf("./ncli -s 1 -p 5000 -c 10");
	exit(0);
}

int send_main(long dport, long count);
int recv_main(long port, long count);

int main(int argc, char** argv) {
	char opt;
	int nopt = 0;
	long count = 1;
	long port = DST_NL_PID;
	long srv = 0;

#if(CMDARGS_EN)
	while (nopt++, ((opt = getopt(argc, argv, "c:p:s:")) != -1))
		switch (opt) {
		case 'p':
			port = strtol(optarg, NULL, 10);
			break;
		case 'c':
			count = strtol(optarg, NULL, 10);
			break;
		case 's':
			srv = strtol(optarg, NULL, 10);
			break;
		case '?':
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

int ex_nl_recvmsg_cb(struct nl_msg *msg, void *arg) {
	struct nlmsghdr *hdr = nlmsg_hdr(msg);
	printf("Recived:%s\n", (char*) NLMSG_DATA(hdr));

	return NL_OK;
}

int recv_main(long sport, long count) {
	int ret;
	struct nl_sock *nls;
	struct nl_cb *cb;

	nls = nl_socket_alloc();
	ret_on_null(nls, "nlsocket alloc");

	nl_socket_set_local_port(nls, sport);
	cb = nl_socket_get_cb(nls);
	nl_cb_set(cb, NL_CB_MSG_IN, NL_CB_CUSTOM, ex_nl_recvmsg_cb, NULL);

	ret = nl_connect(nls, NETLINK_GENERIC);
	ret_on_fail(ret, "nl_connect");

	do {
		nl_recvmsgs_default(nls);
		printf("%ld\n", count);
	} while (--count);

	nl_socket_free(nls);
	return 0;
}

int send_main(long dport, long count) {
	struct nl_sock * nls = nl_socket_alloc();
	struct nl_msg *msg = nlmsg_alloc();
	struct nlmsghdr *hdr;
	char *txt = "hello_msg";

	nl_socket_set_peer_port(nls, dport);

	nl_connect(nls, NETLINK_GENERIC);

	hdr = nlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, NETLINK_GENERIC,
			strlen(txt), NLM_F_CREATE);

	do {
		sprintf(nlmsg_data(hdr), "hello_msg%ld", count);
		nl_send_auto(nls, msg);
	} while (--count);

	nlmsg_free(msg);
	nl_socket_free(nls);

	return 0;
}
