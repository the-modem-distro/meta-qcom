inherit bin_package

DESCRIPTION = "Qualcomm Proprietary blobs and scripts"
LICENSE = "CLOSED"
PROVIDES = "qcom-minimal"
RDEPENDS_${PN} = "glibc openssl libcap"
PACKAGE_WRITE_DEPS = "libcap-native"
MY_PN = "qcom-minimal"
PR = "r1"
S = "${WORKDIR}"
INSANE_SKIP_${PN} = "ldflags"
INSANE_SKIP_${PN} = "file-rdeps"
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_SYSROOT_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
SOLIBS = ".so"
FILES_SOLIBSDEV = ""
SRC_URI="file://usr/bin/alsaucm_test \
         file://usr/bin/qti \
         file://usr/lib/libqmi_cci.so.1.0.0 \
         file://usr/lib/libqmi_client_qmux.so.1.0.0 \
         file://usr/lib/libqmi_encdec.so.1.0.0 \
         file://usr/lib/libqmiservices.so.1.0.0 \
         file://usr/lib/librmnetctl.so.0.0.0 \
         file://usr/lib/libdsutils.so.1.0.0 \
         file://usr/lib/libdiag.so.1.0.0 \
         file://usr/lib/libqcmapipc.so.1.0.0 \
         file://usr/lib/libqcmaputils.so.1.0.0 \
         file://usr/lib/libconfigdb.so.0.0.0 \
         file://usr/lib/libxml.so.0.0.0 \
         file://usr/lib/libtime_genoff.so.1.0.0 \
         file://usr/lib/libalsa_intf.so.1.0.0 \
         file://usr/lib/libacdbloader.so \
         file://etc/init.d/start_qti_le"

do_install() {
      #make folders if they dont exist
      install -d ${D}/bin
      install -d ${D}/lib
      install -d ${D}/usr/bin
      install -d ${D}/usr/lib
      install -d ${D}/etc/init.d
      install -d ${D}/etc/rcS.d
      install -d ${D}/etc/firmware
      install -d ${D}/etc/udhcpc.d
      install -d ${D}/etc/data

      install -d ${D}/persist

      # binaries
      install -m 0755 ${S}/usr/bin/alsaucm_test ${D}/usr/bin
      install -m 0755 ${S}/usr/bin/qti ${D}/usr/bin

      # Libraries
      cp ${S}/usr/lib/libqmi_cci.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqmi_client_qmux.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqmi_encdec.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqmiservices.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/librmnetctl.so.0.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libdsutils.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libdiag.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqcmapipc.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libqcmaputils.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libconfigdb.so.0.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libxml.so.0.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libtime_genoff.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libalsa_intf.so.1.0.0  ${D}/usr/lib/
      cp ${S}/usr/lib/libacdbloader.so  ${D}/usr/lib/

      # services
      install -m 0755 ${S}/etc/init.d/start_qti_le ${D}/etc/init.d/

      # link services on boot
      ln -sf -r ${D}/etc/init.d/start_qti_le ${D}/etc/rcS.d/S40start_qti_le
}

do_install_append() {
      ln -sf -r ${S}/usr/lib/libqmi_cci.so.1.0.0  ${D}/usr/lib/libqmi_cci.so.1
      ln -sf -r ${S}/usr/lib/libqmi_client_qmux.so.1.0.0  ${D}/usr/lib/libqmi_client_qmux.so.1
      ln -sf -r ${S}/usr/lib/libqmi_encdec.so.1.0.0  ${D}/usr/lib/libqmi_encdec.so.1
      ln -sf -r ${S}/usr/lib/libqmiservices.so.1.0.0  ${D}/usr/lib/libqmiservices.so.1
      ln -sf -r ${S}/usr/lib/librmnetctl.so.0.0.0  ${D}/usr/lib/librmnetctl.so.0
      ln -sf -r ${S}/usr/lib/libdsutils.so.1.0.0  ${D}/usr/lib/libdsutils.so.1
      ln -sf -r ${S}/usr/lib/libdiag.so.1.0.0  ${D}/usr/lib/libdiag.so.1
      ln -sf -r ${S}/usr/lib/libqcmapipc.so.1.0.0  ${D}/usr/lib/libqcmapipc.so.1
      ln -sf -r ${S}/usr/lib/libqcmaputils.so.1.0.0  ${D}/usr/lib/libqcmaputils.so.1
      ln -sf -r ${S}/usr/lib/libconfigdb.so.0.0.0  ${D}/usr/lib/libconfigdb.so.0
      ln -sf -r ${S}/usr/lib/libxml.so.0.0.0  ${D}/usr/lib/libxml.so.0
      ln -sf -r ${S}/usr/lib/libtime_genoff.so.1.0.0  ${D}/usr/lib/libtime_genoff.so.1
      ln -sf -r ${S}/usr/lib/libalsa_intf.so.1.0.0  ${D}/usr/lib/libalsa_intf.so.1

}

pkg_postinst_${PN} () {
      setcap cap_net_raw,cap_net_admin,cap_dac_override+eip "$D/usr/bin/qti"
}