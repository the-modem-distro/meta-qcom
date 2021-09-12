inherit bin_package

DESCRIPTION = "Proprietary blobs for the ADSP"
LICENSE = "CLOSED"
PROVIDES = "adsp-firmware"
MY_PN = "adsp-firmware"
PR = "r1"
# ADSP firmware is not ARM ELF binary, skip QA for this
INSANE_SKIP_${PN} = "ldflags arch"
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_SYSROOT_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
SRC_URI="   file://mba.b03 \
            file://modem.b02 \
            file://modem.b10 \
            file://modem.b19 \
            file://mba.b04 \
            file://modem.b03 \
            file://modem.b11 \
            file://modem.b20 \
            file://mba.b05 \
            file://modem.b05 \
            file://modem.b12 \
            file://modem.b21 \
            file://mba.mbn \
            file://modem.b06 \
            file://modem.b13 \
            file://modem.b22 \
            file://mba.b00 \
            file://mba.mdt \
            file://modem.b07 \
            file://modem.b14 \
            file://modem.mdt \
            file://mba.b01 \
            file://modem.b00 \
            file://modem.b08 \
            file://modem.b17 \
            file://mba.b02 \
            file://modem.b01 \
            file://modem.b09 \
            file://modem.b18"

S = "${WORKDIR}"

do_install() {
      #make folders if they dont exist
      install -d ${D}/firmware/modem/image/
      install -m 0755  ${S}/mba.b03  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b02  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b10  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b19  ${D}/firmware/modem/image/
      install -m 0755  ${S}/mba.b04  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b03  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b11  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b20  ${D}/firmware/modem/image/
      install -m 0755  ${S}/mba.b05  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b05  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b12  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b21  ${D}/firmware/modem/image/
      install -m 0755  ${S}/mba.mbn  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b06  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b13  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b22  ${D}/firmware/modem/image/
      install -m 0755  ${S}/mba.b00  ${D}/firmware/modem/image/
      install -m 0755  ${S}/mba.mdt  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b07  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b14  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.mdt  ${D}/firmware/modem/image/
      install -m 0755  ${S}/mba.b01  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b00  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b08  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b17  ${D}/firmware/modem/image/
      install -m 0755  ${S}/mba.b02  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b01  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b09  ${D}/firmware/modem/image/
      install -m 0755  ${S}/modem.b18  ${D}/firmware/modem/image/
}
