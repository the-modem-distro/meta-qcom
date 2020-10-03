inherit bin_package

DESCRIPTION = "Quectel Proprietary blobs and scripts"
LICENSE = "CLOSED"
PROVIDES = "proprietary-libraries"
MY_PN = "proprietary-libraries"
PR = "r1"
INSANE_SKIP_${PN} = "ldflags"
INSANE_SKIP_${PN} = "file-rdeps"
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_SYSROOT_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
SRC_URI="file://libqmi_cci.so.1 \
         file://libeloop.so.1 \
         file://libql_log.so.1 \
         file://libql_utils.so.1 \
         file://libql_mgmt_client.so.1 \
         file://libql_mam_ipc.so.1 \
         file://libdsi_netctrl.so.0 \
         file://libqmiservices.so.1 \
         file://libqmi_csi.so.1 \
         file://libql_usb.so.1 \
         file://libqcmap_client.so.1 \
         file://libqcmapipc.so.1 \
         file://libadns.so.1 \
         file://libconfigdb.so.0 \
         file://libdiag.so.1 \
         file://libdsutils.so.1 \
         file://libnetmgr.so.0 \
         file://libqdi.so.0 \
         file://libql_rawdata.so.1 \
         file://libqmi.so.1 \
         file://libqmi_client_helper.so.1 \
         file://libqmi_client_qmux.so.1  \
         file://libqmi_common_so.so.1 \
         file://libqmi_encdec.so.1 \
         file://libqmi_sap.so.1 \
         file://libqmiidl.so.1 \
         file://librmnetctl.so.0 \
         file://libtime_genoff.so.1 \
         file://libql_tts_eng.so.1 \
         file://libql_lib_audio.so.1 \
         file://libql_misc.so.1 \
         file://libql_lib_audio.so.1 \
         file://libmad.so.0 \
         file://libalsa_intf.so.1 \
         file://libxml.so.0 "


S = "${WORKDIR}/"

do_install() {
      # make folders if they dont exist
      install -d ${D}/lib
      install -d ${D}/usr/lib

      # copy libraries
      cp  ${S}/libqmi_cci.so.1 ${D}/usr/lib
      cp  ${S}/libeloop.so.1 ${D}/usr/lib
      cp  ${S}/libql_log.so.1 ${D}/usr/lib
      cp  ${S}/libql_utils.so.1 ${D}/usr/lib
      cp  ${S}/libql_mgmt_client.so.1 ${D}/usr/lib
      cp  ${S}/libql_mam_ipc.so.1 ${D}/usr/lib
      cp  ${S}/libdsi_netctrl.so.0 ${D}/usr/lib
      cp  ${S}/libqmiservices.so.1 ${D}/usr/lib
      cp  ${S}/libqmi_csi.so.1 ${D}/usr/lib
      cp  ${S}/libql_usb.so.1 ${D}/usr/lib
      cp  ${S}/libqcmap_client.so.1 ${D}/usr/lib
      cp  ${S}/libqcmapipc.so.1 ${D}/usr/lib
      cp  ${S}/libadns.so.1 ${D}/usr/lib
      cp  ${S}/libconfigdb.so.0 ${D}/usr/lib
      cp  ${S}/libdiag.so.1 ${D}/usr/lib
      cp  ${S}/libdsutils.so.1 ${D}/usr/lib
      cp  ${S}/libnetmgr.so.0 ${D}/usr/lib
      cp  ${S}/libqdi.so.0 ${D}/usr/lib
      cp  ${S}/libql_rawdata.so.1 ${D}/usr/lib
      cp  ${S}/libqmi.so.1 ${D}/usr/lib
      cp  ${S}/libqmi_client_helper.so.1 ${D}/usr/lib
      cp  ${S}/libqmi_client_qmux.so.1 ${D}/usr/lib
      cp  ${S}/libqmi_common_so.so.1 ${D}/usr/lib
      cp  ${S}/libqmi_encdec.so.1 ${D}/usr/lib
      cp  ${S}/libqmi_sap.so.1 ${D}/usr/lib
      cp  ${S}/libqmiidl.so.1 ${D}/usr/lib
      cp  ${S}/librmnetctl.so.0 ${D}/usr/lib
      cp  ${S}/libtime_genoff.so.1 ${D}/usr/lib
      cp  ${S}/libxml.so.0 ${D}/usr/lib
      cp  ${S}/libql_tts_eng.so.1 ${D}/usr/lib
      cp  ${S}/libql_lib_audio.so.1 ${D}/usr/lib
      cp  ${S}/libql_lib_audio.so.1 ${D}/usr/lib
      cp  ${S}/libmad.so.0 ${D}/usr/lib
      cp  ${S}/libalsa_intf.so.1 ${D}/usr/lib
      cp  ${S}/libql_misc.so.1 ${D}/usr/lib


      # symlinks 
      ln -sf -r  ${D}/usr/lib/libqmi_cci.so.1 ${D}/usr/lib/libqmi_cci.so
      ln -sf -r  ${D}/usr/lib/libeloop.so.1 ${D}/usr/lib/libeloop.so
      ln -sf -r  ${D}/usr/lib/libql_log.so.1 ${D}/usr/lib/libql_log.so
      ln -sf -r  ${D}/usr/lib/libql_utils.so.1 ${D}/usr/lib/libql_utils.so
      ln -sf -r  ${D}/usr/lib/libql_mgmt_client.so.1 ${D}/usr/lib/libql_mgmt_client.so
      ln -sf -r  ${D}/usr/lib/libql_mam_ipc.so.1 ${D}/usr/lib/libql_mam_ipc.so
      ln -sf -r  ${D}/usr/lib/libdsi_netctrl.so.0 ${D}/usr/lib/libdsi_netctrl.so
      ln -sf -r  ${D}/usr/lib/libqmiservices.so.1 ${D}/usr/lib/libqmiservices.so
      ln -sf -r  ${D}/usr/lib/libqmi_csi.so.1 ${D}/usr/lib/libqmi_csi.so
      ln -sf -r  ${D}/usr/lib/libql_usb.so.1 ${D}/usr/lib/libql_usb.so
      ln -sf -r  ${D}/usr/lib/libqcmap_client.so.1 ${D}/usr/lib/libqcmap_client.so
      ln -sf -r  ${D}/usr/lib/libqcmapipc.so.1 ${D}/usr/lib/libqcmapipc.so
      ln -sf -r  ${D}/usr/lib/libql_tts_eng.so.1 ${D}/usr/lib/libql_tts.so.1
}
