
ZNAME=$(hostname -s);
echo "Snapshot for $ZNAME..."
zip -y -r "SS_$ZNAME.zip" \
	"/sys/" \
	"/sys/devices/" \
	"/sys/bus/" \
	"/sys/block/" \
	"/sys/class/" \
	"/sys/firmware/devicetree/base" \
	"/sys/devices/" \
	"/sys/devices/system/cpu/" \
	"/sys/devices/virtual/dmi/id" \
	"/proc/sys/" \
	"/proc/devicetree/" \
	"/proc/cpuinfo" \
	"/proc/meminfo" \
	"/proc/stat" \
	"/proc/uptime" \
	"/proc/crypto" \
	"/proc/modules" \
	"/usr/lib/os-release" \
    --exclude "/sys/kernel/security/apparmor/*"
