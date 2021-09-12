inherit bin_package

DESCRIPTION = "Qualcomm ADSP calibration profiles"
LICENSE = "CLOSED"
PROVIDES = "calibration-profiles"
MY_PN = "calibration-profiles"
PR = "r1"
# These files aren't ARM ELF binaries, skip QA for this
INSANE_SKIP_${PN} = "ldflags arch"
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_SYSROOT_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
SRC_URI="   file://Bluetooth_cal.acdb \
            file://General_cal.acdb \
            file://Global_cal.acdb \
            file://Handset_cal.acdb \
            file://Hdmi_cal.acdb \
            file://Headset_cal.acdb \
            file://Speaker_cal.acdb"

S = "${WORKDIR}"

do_install() {
      #make folders if they dont exist
      install -d ${D}/etc/acdb/
      install -m 0644  ${S}/Bluetooth_cal.acdb  ${D}/etc/acdb/
      install -m 0644  ${S}/General_cal.acdb  ${D}/etc/acdb/
      install -m 0644  ${S}/Global_cal.acdb  ${D}/etc/acdb/
      install -m 0644  ${S}/Handset_cal.acdb  ${D}/etc/acdb/
      install -m 0644  ${S}/Hdmi_cal.acdb  ${D}/etc/acdb/
      install -m 0644  ${S}/Headset_cal.acdb  ${D}/etc/acdb/
      install -m 0644  ${S}/Speaker_cal.acdb  ${D}/etc/acdb/
}
