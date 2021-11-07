SUMMARY = "Small utility to retrieve temperature sensor data in MDM9607"
LICENSE = "MIT"
MY_PN = "datalogger"
RPROVIDES_${PN} = "datalogger"
PR = "r7"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://src/datalogger.h file://src/datalogger.c"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${LDFLAGS} -O2 src/datalogger.c -o datalogger
}

do_install() {
    install -d ${D}${bindir}

    install -m 0755 ${S}/datalogger ${D}${bindir}
}

pkg_postinst:${PN}() {
   #!/bin/sh
   echo "DL:12345:respawn:/usr/bin/datalogger" >> $D/etc/inittab
}