#include <netlink/route/tc.h>

int main()
{
  struct nl_sock *sock;
  struct rtnl_link *link;
  unsigned int mtu;
  uint64_t rx_packets;
  int ifindex;
  struct nl_cache *qdisc_cache;
  struct nl_dump_params dump_params;
  struct rtnl_qdisc *qdisc;
  uint32_t backlog, qlen;
  FILE *fh;
  char buf[256];
  char *c;
  unsigned long tx_retries_up, tx_retries_down; 
  unsigned long pktdup_count;

  sock = nl_socket_alloc();
  if (!sock) {
    perror("sock ");
    return;
  }

  nl_connect(sock, NETLINK_ROUTE);
 
  /* printf("fd: %d\n", nl_socket_get_fd(sock)); */

  if (rtnl_link_get_kernel(sock, 0, "eth0", &link) < 0) {
    perror("link ");
    goto out_sock;
  }

  mtu = rtnl_link_get_mtu(link);
  printf("mtu: %u\n", mtu);

  /* See /usr/include/libnl3/netlink/route/link.h for other stats */
  rx_packets = rtnl_link_get_stat(link, RTNL_LINK_RX_PACKETS);
  printf("rx_packets: %lu\n", rx_packets);

  ifindex = rtnl_link_get_ifindex(link);

  if (rtnl_qdisc_alloc_cache(sock, &qdisc_cache) < 0) {
    perror("qdisc_cache ");
    goto out_link;
  }
  
  /* memset(&dump_params, 0, sizeof(dump_params)); */
  /* dump_params.dp_fd = stdout; */
  /* dump_params.dp_type = NL_DUMP_DETAILS; */
  /* nl_cache_dump(qdisc_cache, &dump_params); */

  qdisc = rtnl_qdisc_get_by_parent(qdisc_cache, ifindex, TC_H_ROOT);

  if (!qdisc)
    goto out_qdisc;

  backlog = rtnl_tc_get_stat(TC_CAST(qdisc), RTNL_TC_BACKLOG);
  qlen  = rtnl_tc_get_stat(TC_CAST(qdisc), RTNL_TC_QLEN);

  printf("backlog: %u  qlen: %u\n", backlog, qlen);

 out:
  rtnl_qdisc_put(qdisc);
 out_qdisc:
  nl_cache_free(qdisc_cache);
 out_link:
  rtnl_link_put(link);
 out_sock:
  nl_socket_free(sock);

  fh = fopen("/proc/net/nan/tx_retries_down", "r");
  fgets(buf, sizeof(buf), fh);
  sscanf(buf, "%lu", &tx_retries_down);
  fclose(fh);

  fh = fopen("/proc/net/nan/tx_retries_up", "r");
  fgets(buf, sizeof(buf), fh);
  sscanf(buf, "%lu", &tx_retries_up);
  fclose(fh);

  printf("retries DOWN: %lu  retries UP: %lu\n",
	 tx_retries_down, tx_retries_up);

  fh = fopen("/proc/net/cham_pktdup/count", "r");
  fgets(buf, sizeof(buf), fh);
  c = buf;
  while (!isdigit(*c))
    c++;
  sscanf(c, "%lu", &pktdup_count);
  fclose(fh);

  printf("Duplicates: %lu\n", pktdup_count);

  return;
}
