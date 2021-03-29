SUMMARY = "Minimal OpenSource QTI reimplementation for Qualcomm MDM9207 userspace"
LICENSE = "MIT"
MY_PN = "openqti"
RPROVIDES_${PN} = "openqti"
PR = "r7"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://inc/openqti.h \
           file://inc/ipc.h \
           file://inc/ipc_security.h \
           file://inc/devices.h \
           file://inc/audio.h \
           file://inc/atfwd.h \
           file://inc/logger.h \
           file://src/ipc.c \
           file://src/openqti.c \
           file://src/mixer.c \
           file://src/pcm.c \
           file://src/logger.c \
           file://init_openqti"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${LDFLAGS} -O2 src/logger.c src/ipc.c src/mixer.c src/pcm.c src/openqti.c -o openqti
}

do_install() {
    install -d ${D}${bindir}
    install -d ${D}/etc/init.d
    install -d ${D}/etc/rcS.d

    install -m 0755 ${S}/openqti ${D}${bindir}
    install -m 0755 ${S}/init_openqti ${D}/etc/init.d/
    ln -sf -r ${D}/etc/init.d/init_openqti ${D}/etc/rcS.d/S40init_openqti
}
