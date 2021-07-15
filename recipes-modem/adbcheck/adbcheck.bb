SUMMARY = "Minimal OpenSource QTI reimplementation for Qualcomm MDM9207 userspace"
LICENSE = "MIT"
MY_PN = "adbcheck"
RPROVIDES_${PN} = "adbcheck"
PR = "r7"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://src/adbcheck.c"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${LDFLAGS} -O2 src/adbcheck.c -o adbcheck
}

do_install() {
    install -d ${D}${bindir}

    install -m 0755 ${S}/adbcheck ${D}${bindir}
}
