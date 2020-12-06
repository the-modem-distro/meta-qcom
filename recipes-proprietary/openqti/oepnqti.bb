SUMMARY = "Minimal OpenSource QTI reimplementation for Qualcomm MDM9207 userspace"
LICENSE = "MIT"
MY_PN = "openqti"
RPROVIDES_${PN} = "openqti"
PR = "r1"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://openqti.c \
           file://openqti.h"

S = "${WORKDIR}"

do_compile() {
    ${CC} openqti.c -o openqti
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 openqti ${D}${bindir}
}
