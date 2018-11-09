
ZNAME=$(hostname -s);
echo "Snapshot for $ZNAME..."
zip -y -r "SS_$ZNAME.zip" \
	"/sys/devices/" \
	"/sys/bus/" \
	"/sys/firmware/devicetree/base" \
	"/sys/devices/system/cpu/" \
	"/sys/devices/virtual/dmi/id" \
	"/proc/sys/" \
	"/proc/devicetree/" \
	"/proc/cpuinfo" \
	"/proc/meminfo" \
	"/proc/stat" \
	"/proc/crpyto" \
	"/proc/modules" \
	"/usr/lib/os-release"
