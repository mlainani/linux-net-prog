#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <linux/if.h>
#include <linux/ip.h>
#include <errno.h>

#define SIOCSETDODAGID   (SIOCDEVPRIVATE + 0)

/* Set DODAG ID for the RPL tunnel */
void main(int argc, char **argv)
{
	int fd;
	int err;
	struct ifreq ifr;
	unsigned long dagid;
	int ret;

	if (argc !=2) {
	  printf("usage: setdodagid <1-9>\n");
	  return;
	}
	
	dagid = strtoul(argv[1], NULL, 0);
	if (dagid < 1 || dagid > 9) {
	  printf("usage: setdodagid <1-9>\n");
	  return;
	}

	strncpy(ifr.ifr_name, "rpltnl0", IFNAMSIZ);
	ifr.ifr_ifru.ifru_data = (void *)&dagid;
	fd = socket(AF_INET6, SOCK_DGRAM, 0);

	err = ioctl(fd, SIOCSETDODAGID, &ifr);

	if (err)
	  perror("");
}

