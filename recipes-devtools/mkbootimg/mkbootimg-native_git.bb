DESCRIPTION = "Boot image creation tool from Android"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/\
${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"
HOMEPAGE = "http://android.git.kernel.org/?p=platform/system/core.git"
PROVIDES = "mkbootimg-native"

inherit base native
PR = "r6"

MY_PN = "mkbootimg"


do_compile() {
         ${CC}  ${CFLAGS} ${LDFLAGS} ${THISDIR}/files/mkbootimg.c -o mkbootimg -Wno-discarded-qualifiers -lmincrypt  --verbose
}

do_install() {
         install -d ${D}${bindir}
         install -m 0755 mkbootimg ${D}${bindir}
}
