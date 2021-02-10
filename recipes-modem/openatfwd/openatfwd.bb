SUMMARY = "Minimal OpenSource AT Forwarder daemon reimplementation for Qualcomm MDM9207 userspace"
LICENSE = "MIT"
MY_PN = "openatfwd"
RPROVIDES_${PN} = "openatfwd"
PR = "r7"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://atfwd.c \
           file://atfwd.h \
           file://qmi_helpers.c \
           file://init_atfwd"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${LDFLAGS} qmi_helpers.c atfwd.c -o atfwd
}

do_install() {
    install -d ${D}${bindir}
    install -d ${D}/etc/init.d
    install -d ${D}/etc/rcS.d

    install -m 0755 ${S}/atfwd ${D}${bindir}
    install -m 0755 ${S}/init_atfwd ${D}/etc/init.d/
    ln -sf -r ${D}/etc/init.d/init_atfwd ${D}/etc/rcS.d/S30init_atfwd
}
