SUMMARY = "Minimal OpenSource QTI reimplementation for Qualcomm MDM9207 userspace"
LICENSE = "MIT"
MY_PN = "openqti"
RPROVIDES_${PN} = "openqti"
PR = "r7"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://inc/openqti.h \
           file://inc/tracking.h \
           file://inc/ipc.h \
           file://inc/devices.h \
           file://inc/audio.h \
           file://inc/atfwd.h \
           file://inc/logger.h \
           file://inc/helpers.h \
           file://inc/qmi.h \
           file://inc/sms.h \
           file://src/tracking.c \
           file://src/helpers.c \
           file://src/atfwd.c \
           file://src/ipc.c \
           file://src/audio.c \
           file://src/openqti.c \
           file://src/mixer.c \
           file://src/pcm.c \
           file://inc/adspfw.h \
           file://inc/md5sum.h \
           file://src/md5sum.c \
           file://src/logger.c \
           file://src/sms.c \
           file://init_openqti \
           file://external/ring8k.wav"

S = "${WORKDIR}"
FILES:${PN} += "/usr/share/tones/*"
do_compile() {
    ${CC} ${LDFLAGS} -O2 src/sms.c src/tracking.c src/helpers.c src/atfwd.c src/logger.c src/md5sum.c src/ipc.c src/audio.c src/mixer.c src/pcm.c src/openqti.c -o openqti -lpthread
}

do_install() {
    install -d ${D}${bindir}
    install -d ${D}/etc/init.d
    install -d ${D}/etc/rcS.d
    install -d ${D}/usr/share/tones/

    install -m 0755 ${S}/openqti ${D}${bindir}
    install -m 0755 ${S}/init_openqti ${D}/etc/init.d/

    # default dialing tone
    install -m 0644 ${S}/external/ring8k.wav ${D}/usr/share/tones/

  #  ln -sf -r ${D}/etc/init.d/init_openqti ${D}/etc/rcS.d/S40init_openqti
}

#  If debugging, make sure you add '-l' to openqti command line inside inittab
#  Otherwise remove it to not fill the ramdisk with garbage logs you are not 
#  going to look at
#

pkg_postinst:${PN}() {
   #!/bin/sh
   # echo "OQ:12345:respawn:/usr/bin/openqti -l" >> $D/etc/inittab
   echo "OQ:12345:respawn:/usr/bin/openqti" >> $D/etc/inittab
}