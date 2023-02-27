# Custom dnsmasq

# Files directory
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# Sources
SRC_URI:append = " file://dnsmasq.conf"

# Add our interface file with rndis support
do_install:append() {
	install -m 644 ${WORKDIR}/dnsmasq.conf ${D}${sysconfdir}/
}