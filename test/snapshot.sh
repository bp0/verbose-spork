
ZNAME=$(hostname -s);
echo "Snapshot for $ZNAME..."
zip -y -r "SS_$ZNAME.zip" \
	/sys \
	/sys/devices \
	/sys/bus \
	/sys/block \
	/sys/class \
	/sys/firmware/devicetree/base \
	/sys/devices \
	/sys/devices/system/cpu/ \
	/sys/devices/virtual/dmi/id \
	/proc/sys \
	/proc/devicetree \
	/proc/cpuinfo \
	/proc/meminfo \
	/proc/stat \
	/proc/mdstat \
	/proc/uptime \
	/proc/crypto \
	/proc/modules \
	/proc/driver \
	/proc/pmu \
	/proc/acpi \
	/proc/scsi \
	/proc/xen \
	/proc/ide \
	/proc/omnibook \
	/proc/ioports \
	/proc/iomem \
	/proc/dma \
	/usr/lib/os-release \
	/etc/issue \
	/etc/*-release \
	/etc/*-version \
	/etc/*_version \
    --exclude "/sys/kernel/security/apparmor/*"
# /proc/net is a symlink into /proc/self, itself a symlink
# into /proc/{PID of zip}, which is useless for snapshot, so
# just store the files instead of the link
zip -r "SS_$ZNAME.zip" /proc/net/*
