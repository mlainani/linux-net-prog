#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/icmp6.h>
#include <netinet/in.h>

#include <linux/mroute6.h>

/*
	NOTE: this sample program has been developped to run on a DODAG
	root. The input/output interfaces and source address for the multicast
	packet need to be adapted for a node. It initializes the kernel
	forwarding engine, adds eth0 and rpltnl0 to the kernel list of
	multicast interface and finally, adds a mcast routing cache entry to
	forward mcast packets received on eth0 to rpltnl0 if their group is
	ff0e:101 and their source address is 2601::aaaa.

 	       	 -------
	eth0	|     	| rpltnl0
        --------| Root 	|--------
  	input  	|	| output
  	       	 -------

  	       	 -------
  	nan0	|     	| rpltnl0
        --------| Node 	|--------
	input  	|	| output
	       	 -------

*/

#define NOT_RPLD	1

int main(int argc, char **argv)
{
     int sockfd;
     const int val = 1;
     struct icmp6_filter filt;
     struct mif6ctl vifc;		/* mcast forwarding engine interface */
     struct mf6cctl mfc;		/* mcast forwarding cache entry */
     struct sockaddr_in6 src;
     struct sockaddr_in6 grp;

     if ((sockfd = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) < 0) {
	  perror("socket()");
	  exit(-1);
     }

#if defined(NOT_RPLD)
     /* Needed to avoid filling up the socket queue while we sleep. */
     /* We block everthing but RPL daemon needs to receive ICMPv6 messages. */
     memset(&filt, 0, sizeof(filt));
     ICMP6_FILTER_SETBLOCKALL(&filt);

     if (setsockopt(sockfd, IPPROTO_ICMPV6, ICMP6_FILTER, (void *)&filt, sizeof(filt))) {
	  perror("setsockopt()");
	  close(sockfd);
	  exit(-1);
     }
#endif

     /* Enable multicast forwarding in the kernel */
     if (setsockopt(sockfd, IPPROTO_IPV6, MRT6_INIT, (void *)&val, sizeof(val))) {
	  perror("setsockopt()");
	  close(sockfd);
	  exit(-1);
     }

     /*
      * NOTE: a real mcast routing daemon would need to maintain a mapping
      * between each interface's ifindex and its multicast interface table
      * index. Whether on the DODAG root or on a node we should only be
      * dealing with 2 interfaces: respectively eth0/rpltnl0 and
      * nan0/rpltnl0. It's up to the RPL daemon developer to decide if we can
      * go for a simple scheme where the Up interface has multicast interface
      * table index 0 and the Down interface has index 1, or if we need a more
      * sophisticated approach.
      */

     /* Adding the first interface to the kernel mcast forwarding engine */
     memset(&vifc, 0, sizeof(vifc));
     vifc.mif6c_mifi = 0; /* index in the kernel multicast interfaces table [0-31] */
     vifc.mif6c_flags = 0;
     vifc.vifc_threshold = 1;
     vifc.mif6c_pifi = if_nametoindex("eth0");
     vifc.vifc_rate_limit = 0;

     if (setsockopt(sockfd, IPPROTO_IPV6, MRT6_ADD_MIF, (char *)&vifc, sizeof(vifc))) {
	  perror("setsockopt()");
	  setsockopt(sockfd, IPPROTO_IPV6, MRT6_DONE, NULL, 0);
	  close(sockfd);
	  exit(-1);
     }

     /* Adding the second interface to the kernel mcast forwarding engine */
     vifc.mif6c_mifi = 1;
     vifc.mif6c_pifi = if_nametoindex("rpltnl0");

     if (setsockopt(sockfd, IPPROTO_IPV6, MRT6_ADD_MIF, (char *)&vifc, sizeof(vifc))) {
	  perror("setsockopt()");
	  setsockopt(sockfd, IPPROTO_IPV6, MRT6_DONE, NULL, 0);
	  close(sockfd);
	  exit(-1);
     }

     /* Adding the multicast route */
     memset(&src, 0, sizeof(src));
     src.sin6_family = AF_INET6;
#if 0
     src.sin6_addr = in6addr_any;	/* We have a potential problem w/ any source-mcast */
#endif
     inet_pton(AF_INET6, "2601::aaaa", &src.sin6_addr);

     memset(&grp, 0, sizeof(grp));
     grp.sin6_family = AF_INET6;
     inet_pton(AF_INET6, "ff3e::1", &grp.sin6_addr);

     memset(&mfc, 0, sizeof(mfc));
     mfc.mf6cc_origin = src;
     mfc.mf6cc_mcastgrp = grp;
     mfc.mf6cc_parent = 0;		/* Input interface is eth0 */
     IF_SET(1, &mfc.mf6cc_ifset);	/* Output interface is rpltnl0 */

     if (setsockopt(sockfd, IPPROTO_IPV6, MRT6_ADD_MFC, (void *)&mfc, sizeof(mfc))) {
	  perror("setsockopt()");
	  setsockopt(sockfd, IPPROTO_IPV6, MRT6_DONE, NULL, 0);
	  close(sockfd);
	  exit(-1);
     }

     sleep(240);

     /* Deleting the second interface from the kernel mcast forwarding engine */
     setsockopt(sockfd, IPPROTO_IPV6, MRT6_DEL_MIF, (char *)&vifc, sizeof(vifc));

     /* Deleting the first interface from the kernel mcast forwarding engine */
     vifc.mif6c_mifi = 0;
     vifc.mif6c_pifi = if_nametoindex("eth0");
     setsockopt(sockfd, IPPROTO_IPV6, MRT6_DEL_MIF, (char *)&vifc, sizeof(vifc));

     setsockopt(sockfd, IPPROTO_IPV6, MRT6_DONE, NULL, 0);
     close(sockfd);

     exit(0);
}
