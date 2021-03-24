SUMMARY = "Minimal OpenSource QTI reimplementation for Qualcomm MDM9207 userspace"
LICENSE = "MIT"
MY_PN = "openqti_"
RPROVIDES_${PN} = "openqti_"
PR = "r7"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://openqti.c \
           file://openqti.h \
           file://audio.h \
           file://mixer.c \
           file://pcm.c \
           file://init_openqti"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${LDFLAGS} mixer.c pcm.c openqti.c -o openqti
}

do_install() {
    install -d ${D}${bindir}
    install -d ${D}/etc/init.d
    install -d ${D}/etc/rcS.d

    install -m 0755 ${S}/openqti ${D}${bindir}
    install -m 0755 ${S}/init_openqti ${D}/etc/init.d/
    ln -sf -r ${D}/etc/init.d/init_openqti ${D}/etc/rcS.d/S20init_openqti
}
