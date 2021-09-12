SUMMARY = "Small utility to send ACDB data to the modem"
LICENSE = "MIT"
MY_PN = "calibloader"
RPROVIDES_${PN} = "calibloader"
TOOLCHAIN_TARGET_TASK_append = " kernel-devsrc"

PR = "r7"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://src/calibloader.h \
           file://src/calibloader.c"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${LDFLAGS}  -O2 src/calibloader.c -o calibloader
}

do_install() {
    install -d ${D}${bindir}

    install -m 0755 ${S}/calibloader ${D}${bindir}
}
