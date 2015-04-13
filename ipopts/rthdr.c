#include <netinet/in.h>
#include <netinet/ip6.h>
#include <string.h>
#include <stdio.h>

#define	MAXLINE				256	/* max text line length */
#define SERV_PORT 			9877

/*
 *	IPv6 TLV options. (from linux/ipv6.h)
 */
#define IPV6_TLV_PAD0			0
#define IPV6_TLV_PADN			1
#define IPV6_TLV_RPL			99

/*
 *	RPL Option sub-TLVs options.
 */
#define IPV6_RPL_TLV_SEQNUM		1
#define IPV6_RPL_TLV_PANID		2
#define IPV6_RPL_TLV_EXPTIME		3
#define IPV6_RPL_TLV_CREATETIME		4
#define IPV6_RPL_TLV_NUMRETRY		5

#define CHAM_DWNSTREAM_HOPOPT_SIZE	40

struct ipv6_hopopt_rpl {
	unsigned char type;
	unsigned char len;
	unsigned char flags;
	unsigned char instance_id;
	unsigned short sender_rank;
	unsigned char sub_tlvs[0];
#define RPL_OPT_F_DIR_DOWN		0x80
#define RPL_OPT_F_RANK_ERR		0x40
#define RPL_OPT_F_FWRD_ERR		0x20
};

/* Number of retries */
struct ipv6_rpl_opt_tlv8 {
	unsigned char type;
	unsigned char len;
	unsigned char val;
};

/* Sequence number, PAN ID */
struct ipv6_rpl_opt_tlv16 {
	unsigned char type;
	unsigned char len;
	unsigned short val;
};

/* Expiration Time */
struct ipv6_rpl_opt_tlv32 {
	unsigned char type;
	unsigned char len;
	unsigned long val;
};

/* Creation Time */
struct ipv6_rpl_opt_tlv48 {
	unsigned char type;
	unsigned char len;
	unsigned char val[6];
};

struct ipv6_padn_opt {
	unsigned char type;
	unsigned char length;
};

/*
 *	routing header (from linux/ipv6.h)
 */
struct ipv6_rt_hdr {
	unsigned char nexthdr;
	unsigned char hdrlen;
	unsigned char type;
	unsigned char segments_left;

	/*
	 *	type specific data
	 *	variable length field
	 */
};

struct rt0_hdr {
	struct ipv6_rt_hdr rt_hdr;
	unsigned int reserved;
	struct in6_addr addr[0];

#define rt0_type rt_hdr.type
};

#define IPV6_RTHDR_TYPE_3		3

/*
 * Since an application has no means to predict the number of segments
 * that will comprise the vector of addresses, it has to provision for
 * the maximum possible size. This could be optimized by making the
 * current maximum depth in the DODAG tree available to userland through
 * a system call.
*/
#define RPL_SRH_PATH_MAXHOPS		32
#define RPL_SRH_SEGMENT_SIZE		2
#define RPL_SRH_ADDRVEC_MAXSIZE 	(RPL_SRH_PATH_MAXHOPS * RPL_SRH_SEGMENT_SIZE)

#define CHAM_DWNSTREAM_RTHDROPT_SIZE	sizeof(struct rt0_hdr) + RPL_SRH_ADDRVEC_MAXSIZE

void
main(int argc, char **argv)
{
  struct sockaddr_in6 addr;
  int ret;
  char mesg[MAXLINE];
  struct iovec iov[1];
  struct msghdr msg;
  char control[CMSG_SPACE(CHAM_DWNSTREAM_HOPOPT_SIZE) +
	       CMSG_SPACE(CHAM_DWNSTREAM_RTHDROPT_SIZE)];
  struct cmsghdr *cmsg;
  struct ip6_ext *opt;
  struct ipv6_hopopt_rpl *rpl;
  struct ipv6_rpl_opt_tlv8 *numretry;
  struct ipv6_rpl_opt_tlv16 *seqnum, *panid;
  struct ipv6_rpl_opt_tlv32 *exptime;
  struct ipv6_rpl_opt_tlv48 *createtime;
  struct rt0_hdr *rt0hdr;
  struct ipv6_padn_opt *padn;
  int sockfd;

  if (argc !=2) {
    printf("usage: rthdr <IPv6address>\n");
    return;
  }
    
  bzero(&addr, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port =  htons(SERV_PORT);
  ret = inet_pton(AF_INET6, argv[1], &addr.sin6_addr);
   if (ret != 1) {
    printf("Invalid IPv6 address\n");
    return;
  }

  iov[0].iov_base = mesg;
  iov[0].iov_len = MAXLINE;
  memset(iov[0].iov_base, 0xa, MAXLINE);

  bzero(&msg, sizeof(msg));
  msg.msg_name = &addr;
  msg.msg_namelen = sizeof(addr);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  bzero(control, sizeof(control));
  msg.msg_control = control;
  msg.msg_controllen = sizeof(control); /* Must be >= sizeof(struct cmsghdr) */

  /*
   * Hop-by-Hop Options header
   */

  cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = IPPROTO_IPV6;
  cmsg->cmsg_type = IPV6_HOPOPTS;
  cmsg->cmsg_len = CMSG_LEN(CHAM_DWNSTREAM_HOPOPT_SIZE);

  /* Hop-by-Hop Options header */
  opt = (struct ip6_ext *) CMSG_DATA(cmsg);
  opt->ip6e_len = 4;	/* length in units of 8 bytes not including the first 8 bytes */

  /* RPL Instance ID is filled by the Linux kernel */
  rpl = (struct ipv6_hopopt_rpl *)((unsigned char *)opt + 2);
  rpl->type = IPV6_TLV_RPL;
  rpl->len = 29;			/* length in bytes - padding bytes excluded */
  rpl->flags |= RPL_OPT_F_DIR_DOWN;
  rpl->sender_rank = 0x0;  /* Set to zero by the source - RFC 6550 - section 11.2 */

  /* Itron-proprietary sub-TLVs: Sequence Number */
  /* Value filled by the Linux kernel */
  seqnum = (struct ipv6_rpl_opt_tlv16 *)((unsigned char *)rpl + 6);
  seqnum->type = IPV6_RPL_TLV_SEQNUM;
  seqnum->len = 2;

  /* Itron-proprietary sub-TLVs: PAN ID */
  /* Value filled by the Linux kernel */
  panid = (struct ipv6_rpl_opt_tlv16 *)(((unsigned char *)seqnum + 4));
  panid->type = IPV6_RPL_TLV_PANID;
  panid->len = 2;
 
  /* Itron-proprietary sub-TLVs: Expiration Time */
  /* Value filled by the Linux kernel */
  exptime = (struct ipv6_rpl_opt_tlv32 *)(((unsigned char *)panid + 4));
  exptime->type = IPV6_RPL_TLV_EXPTIME;
  exptime->len = 4;
  
  /* Itron-proprietary sub-TLVs: Creation Time*/
  /* Value filled by the Linux kernel */
  createtime = (struct ipv6_rpl_opt_tlv48 *)((unsigned char *)exptime + 6);
  createtime->type = IPV6_RPL_TLV_CREATETIME;
  createtime->len = 6;
  
  /* Itron-proprietary sub-TLVs: number of retries */
  /* Value filled by the Linux kernel */
  numretry = (struct ipv6_rpl_opt_tlv8 *)(((unsigned char *)createtime + 8));
  numretry->type = IPV6_RPL_TLV_NUMRETRY;
  numretry->len = 1;
 
  /* 7-byte padding */
  padn = (struct ipv6_padn_opt *)((unsigned char *)numretry + 3);
  padn->type = IPV6_TLV_PADN;
  padn->length = 5;

  /*
   * Routing header
   * Computed and filled by the Linux kernel
   */

  cmsg = CMSG_NXTHDR(&msg, cmsg);
  cmsg->cmsg_level = IPPROTO_IPV6;
  cmsg->cmsg_type = IPV6_RTHDR;
  cmsg->cmsg_len = CMSG_LEN(CHAM_DWNSTREAM_RTHDROPT_SIZE);

  rt0hdr = (struct rt0_hdr *) CMSG_DATA(cmsg);
  rt0hdr->rt0_type = IPV6_RTHDR_TYPE_3;

  {
    int i;
    printf("\n");
    for (i = 0; i < sizeof(control); i++) {
      printf("0x%.2X ", control[i]);
      if ((i + 1) % 4 == 0)
	printf("\n");
    }
    printf("\n");
  }

  sockfd = socket(AF_INET6, SOCK_DGRAM, 0);

  sendmsg(sockfd, &msg, 0);

  perror("");
}
