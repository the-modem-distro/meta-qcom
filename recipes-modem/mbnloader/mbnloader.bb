SUMMARY = "Small utility to sync current date from network"
LICENSE = "MIT"
MY_PN = "mbnloader"
RPROVIDES_${PN} = "mbnloader"
PR = "r1"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://src/mbnloader.h file://src/mbnloader.c"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${LDFLAGS} -O2 src/mbnloader.c -o mbnloader
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${S}/mbnloader ${D}${bindir}
}

pkg_postinst:${PN}() {
   #!/bin/sh
   echo "TS:12345:boot:/usr/bin/mbnloader" >> $D/etc/inittab
}