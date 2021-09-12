SUMMARY = "Small utility to check if USB Audio flag is set in the misc partition"
LICENSE = "MIT"
MY_PN = "audiocheck"
RPROVIDES_${PN} = "audiocheck"
PR = "r7"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://src/audiocheck.c"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${LDFLAGS} -O2 src/audiocheck.c -o audiocheck
}

do_install() {
    install -d ${D}${bindir}

    install -m 0755 ${S}/audiocheck ${D}${bindir}
}
