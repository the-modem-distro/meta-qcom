# Custom ifupdown

# Files directory
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# Sources
SRC_URI:append = " file://interfaces"

# Add our interface file with rndis support
do_install:append() {
	install -m 0644 ${S}/interfaces ${D}${sysconfdir}/network/
}