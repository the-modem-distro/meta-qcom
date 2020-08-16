inherit native

PR = "r4"

MY_PN = "dtbtool"

DESCRIPTION = "Boot image creation tool from Android"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/\
${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"
PROVIDES = "dtbtool-native"

# Handle do_fetch ourselves...  The automated tools don't work nicely with this...
do_fetch () {
	install -d ${S}
	cp -rf $OEROOT/android_compat/device/qcom/common/${MY_PN}/* ${S}
	cp -f ${THISDIR}/files/makefile ${S}
}

do_fetch_msm8974 () {
        install -d ${S}
        cp -rf $OEROOT/system/core/${MY_PN}/* ${S}
        cp -f ${THISDIR}/files/makefile ${S}
}

do_install() {
	install -d ${D}${bindir}
	install ${MY_PN} ${D}${bindir}
}

NATIVE_INSTALL_WORKS = "1"
