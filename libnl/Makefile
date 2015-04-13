CROSS_COMPILE := /home/mlainani/factory/build_armv7l-timesys-linux-uclibcgnueabi/toolchain/bin/armv7l-timesys-linux-uclibcgnueabi-
CC := $(CROSS_COMPILE)gcc
INCLUDE_DIRS := /home/mlainani/factory/build_armv7l-timesys-linux-uclibcgnueabi/toolchain/include/libnl3

qdisc: qdisc.c
	$(CC) -o qdisc qdisc.c -I$(INCLUDE_DIRS) -lnl-route-3 -lnl-3

clean:
	rm -f qdisc *~