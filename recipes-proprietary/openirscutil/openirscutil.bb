SUMMARY = "Minimal OpenSource IPC Router Security Feeder reimplementation for Qualcomm MDM9207 userspace"
LICENSE = "MIT"
MY_PN = "openirscutil"
RPROVIDES_${PN} = "openirscutil"
PR = "r5"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://openirscutil.c \
           file://init_openirscutil"

S = "${WORKDIR}"

do_compile() {
    ${CC} openirscutil.c -o openirscutil
}

do_install() {
    install -d ${D}${bindir}
    install -d ${D}/etc/init.d
    install -d ${D}/etc/rcS.d

    install -m 0755 ${S}/openirscutil ${D}${bindir}
    install -m 0755 ${S}/init_openirscutil ${D}/etc/init.d/
    ln -sf -r ${D}/etc/init.d/init_openirscutil ${D}/etc/rcS.d/S20init_openirscutil


}
