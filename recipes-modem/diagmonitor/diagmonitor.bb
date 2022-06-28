SUMMARY = "Small utility to retrieve temperature sensor data in MDM9607"
LICENSE = "MIT"
MY_PN = "diagmonitor"
RPROVIDES_${PN} = "diagmonitor"
PR = "r7"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://src/diagmonitor.h file://src/diagmonitor.c"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${LDFLAGS} -O2 src/diagmonitor.c -o diagmonitor
}

do_install() {
    install -d ${D}${bindir}

    install -m 0755 ${S}/diagmonitor ${D}${bindir}
}

pkg_postinst:${PN}() {
   #!/bin/sh
   echo "DL:12345:respawn:/usr/bin/diagmonitor" >> $D/etc/inittab
}