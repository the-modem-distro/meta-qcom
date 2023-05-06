DESCRIPTION = "Boot image creation tool from Android"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/\
${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"
HOMEPAGE = "http://android.git.kernel.org/?p=platform/system/core.git"
PROVIDES = "mkbootimg-native"

inherit native
PR = "r6"

MY_PN = "mkbootimg"

SRC_URI = "file://mkbootimg.c \
           file://sha256.c \
           file://sha256.h \
           file://sha.h \
           file://sha.c \
           file://bootimg.h \
           file://hash-internal.h"

S = "${WORKDIR}"

do_compile() {
         ${CC}  sha256.c mkbootimg.c  -o mkbootimg -Wno-discarded-qualifiers  --verbose
}

do_install() {
         install -d ${D}${bindir}
         install -m 0755 mkbootimg ${D}${bindir}
}
