#!/bin/sh
#
# Octeon platform hook for /etc/init.d/packet_steering ($1 = the
# network.globals.packet_steering value).
#
# PA-220: the ethernet driver spreads received flows over one hardware
# work queue per core (POW groups, see octeon_ethernet.receive_group_order).
# Pin each queue's "Ethernet" IRQ to its own CPU so the four cores share
# the load, then apply OpenWrt's normal (RPS/XPS) packet steering setting.
#

GENERIC=/usr/libexec/network/packet-steering.uc

if [ "$(cat /tmp/sysinfo/board_name 2>/dev/null)" = "pan,pa-220" ]; then
	ncpu=$(grep -c '^processor' /proc/cpuinfo)
	i=0
	for irq in $(awk -F: '/Ethernet/ { gsub(/ /, "", $1); print $1 }' /proc/interrupts); do
		printf '%x' $((1 << (i % ncpu))) > "/proc/irq/$irq/smp_affinity" 2>/dev/null
		i=$((i + 1))
	done
fi

exec "$GENERIC" "$@"
