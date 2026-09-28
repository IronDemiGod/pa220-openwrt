#!/bin/sh
# Copyright (C) 2006-2019 OpenWrt.org

. /lib/functions/leds.sh

boot="$(get_dt_led boot)"
failsafe="$(get_dt_led failsafe)"
running="$(get_dt_led running)"
upgrade="$(get_dt_led upgrade)"

set_led_state() {
	status_led="$boot"

	# PA-220: the boot LED (STAT orange) is solid while booting instead of
	# blinking; failsafe and upgrade still blink. Checked here, not at
	# source time, because preinit sources this before board_name is set.
	local boot_solid=
	[ "$(cat /tmp/sysinfo/board_name 2>/dev/null)" = "pan,pa-220" ] && boot_solid=1

	case "$1" in
	preinit)
		if [ -n "$boot_solid" ]; then
			status_led_on
		else
			status_led_blink_preinit
		fi
		;;
	failsafe)
		status_led_off
		[ -n "$running" ] && {
			status_led="$running"
			status_led_off
		}
		status_led="$failsafe"
		status_led_blink_failsafe
		;;
	preinit_regular)
		if [ -n "$boot_solid" ]; then
			status_led_on
		else
			status_led_blink_preinit_regular
		fi
		;;
	upgrade)
		[ -n "$running" ] && {
			status_led="$running"
			status_led_off
		}
		status_led="$upgrade"
		status_led_blink_preinit_regular
		;;
	done)
		status_led_off
		[ "$status_led" != "$running" ] && \
			status_led_restore_trigger "boot"
		[ -n "$running" ] && {
			status_led="$running"
			status_led_on
		}
		;;
	esac
}

set_state() {
	[ -n "$boot" -o -n "$failsafe" -o -n "$running" -o -n "$upgrade" ] && set_led_state "$1"
}
