#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>
#include <net/if.h>

#define	MAXLINE		8	/* max text line length */
#define SERV_PORT	9877

int
main(void)
{
  unsigned int ifindex, tnl_ifindex;
  int sockfd;
  struct sockaddr_in6 addr, saddr, grpaddr;
  int n;
  unsigned char payload[MAXLINE];
  socklen_t len;
  struct ipv6_mreq mreq6;

  ifindex = if_nametoindex("nan0");
  tnl_ifindex = if_nametoindex("rpltnl0");
  if (!ifindex || !tnl_ifindex) {
    printf("NAN device or IPv6-in-IPv6 tunnel interface not found\n");
    exit(-1);
  }

  sockfd = socket(AF_INET6, SOCK_DGRAM, 0);

  bzero(&addr, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_addr = in6addr_any;
  addr.sin6_port =  htons(SERV_PORT);

  bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
 
  inet_pton(AF_INET6, "ff0e:0:0:0:0:0:0:101", &grpaddr.sin6_addr);
  memcpy(&mreq6.ipv6mr_multiaddr,
	 &((const struct sockaddr_in6 *)&grpaddr)->sin6_addr,
	 sizeof(struct in6_addr));
  mreq6.ipv6mr_interface = ifindex;

  setsockopt(sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
	     &mreq6, sizeof(mreq6));

  mreq6.ipv6mr_interface = tnl_ifindex;
  setsockopt(sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
	     &mreq6, sizeof(mreq6));

  for ( ; ; ) {
    n = recvfrom(sockfd, payload, MAXLINE, 0, (struct sockaddr *)&saddr, &len);
    if (n < 0) {
      perror("");
      exit(-1);
    }
    else
      printf("Received %d bytes\n", n);
  }

  exit(0);
}
