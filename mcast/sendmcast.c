#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip6.h>

#define	MAXLINE		8	/* max text line length */
#define SERV_PORT	9877

struct in6_pktinfo
{
     struct in6_addr ipi6_addr;	/* src/dst IPv6 address */
     unsigned int ipi6_ifindex;	/* send/recv interface index */
};

int main(int argc, char **argv)
{
     unsigned int ifindex;
     int ret;
     unsigned char payload[MAXLINE];
     struct iovec iov[1];
     struct msghdr msg;
     struct sockaddr_in6 addr;
     unsigned char control[CMSG_SPACE(sizeof(struct in6_pktinfo))];
     struct cmsghdr *cmsg;
     struct in6_pktinfo *pi;
     int sockfd;
     int hoplimit = 3;
 
     if (argc != 3) {
	  printf("usage: sendmcast <ifname> <groupaddr>\n");
	  exit(-1);
     }

     ifindex = if_nametoindex(argv[1]);
     if (!ifindex) {
	  perror("if_nametoindex");
	  exit(-1);
     }

     bzero(&addr, sizeof(addr));
     addr.sin6_family = AF_INET6;
     addr.sin6_port =  htons(SERV_PORT);
     ret = inet_pton(AF_INET6, argv[2], &addr.sin6_addr);
     if (ret == 0) {
	  printf("%s is not a valid IPv6 address\n", argv[2]);
	  exit(-1);
     }
     else if (ret == -1) {
	  perror("inet_pton");
	  exit(-1);
     }
  
     if (!IN6_IS_ADDR_MULTICAST(&addr.sin6_addr)) {
	  printf("%s is not a valid mcast group address\n", argv[2]);
	  exit(-1);
     }

     iov[0].iov_base = payload;
     iov[0].iov_len = MAXLINE;
     memset(iov[0].iov_base, 0xa, MAXLINE);

     bzero(&msg, sizeof(msg));
     msg.msg_name = &addr;
     msg.msg_namelen = sizeof(addr);
     msg.msg_iov = iov;
     msg.msg_iovlen = 1;

     bzero(control, sizeof(control));
     msg.msg_control = control;
     msg.msg_controllen = sizeof(control); /* Must be > sizeof(struct cmsghdr) */

     cmsg = CMSG_FIRSTHDR(&msg);
     cmsg->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));
     cmsg->cmsg_level = IPPROTO_IPV6;
     cmsg->cmsg_type = IPV6_PKTINFO;
     pi = (struct in6_pktinfo *)CMSG_DATA(cmsg);
     pi->ipi6_ifindex = ifindex;

     sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
     setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hoplimit, sizeof(hoplimit));

     sendmsg(sockfd, &msg, 0);
     perror("");

     exit(0);
}
