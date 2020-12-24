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
SRC_URI="file://libeloop.so.1 \
         file://libql_log.so.1 \
         file://libql_utils.so.1 \
         file://libql_mgmt_client.so.1 \
         file://libql_mam_ipc.so.1 \
         file://libqmi_csi.so.1 \
         file://libql_usb.so.1 \
         file://libqcmapipc.so.1 \
         file://libadns.so.1 \
         file://libql_rawdata.so.1 \
         file://librmnetctl.so.0 \
         file://libql_tts_eng.so.1 \
         file://libql_lib_audio.so.1 \
         file://libql_misc.so.1 \
         file://libql_lib_audio.so.1 \
         file://libmad.so.0 \
         file://libalsa_intf.so.1 "


S = "${WORKDIR}"

do_install() {
      # make folders if they dont exist
      install -d ${D}/lib
      install -d ${D}/usr/lib

      # copy libraries
      cp  ${S}/libeloop.so.1 ${D}/usr/lib
      cp  ${S}/libql_log.so.1 ${D}/usr/lib
      cp  ${S}/libql_utils.so.1 ${D}/usr/lib
      cp  ${S}/libql_mgmt_client.so.1 ${D}/usr/lib
      cp  ${S}/libql_mam_ipc.so.1 ${D}/usr/lib
      cp  ${S}/libql_usb.so.1 ${D}/usr/lib
      cp  ${S}/libqcmapipc.so.1 ${D}/usr/lib
      cp  ${S}/libadns.so.1 ${D}/usr/lib
      cp  ${S}/libql_rawdata.so.1 ${D}/usr/lib
      cp  ${S}/librmnetctl.so.0 ${D}/usr/lib
      cp  ${S}/libql_tts_eng.so.1 ${D}/usr/lib
      cp  ${S}/libql_lib_audio.so.1 ${D}/usr/lib
      cp  ${S}/libql_lib_audio.so.1 ${D}/usr/lib
      cp  ${S}/libmad.so.0 ${D}/usr/lib
      cp  ${S}/libalsa_intf.so.1 ${D}/usr/lib
      cp  ${S}/libql_misc.so.1 ${D}/usr/lib


      # symlinks 
      ln -sf -r  ${D}/usr/lib/libeloop.so.1 ${D}/usr/lib/libeloop.so
      ln -sf -r  ${D}/usr/lib/libql_log.so.1 ${D}/usr/lib/libql_log.so
      ln -sf -r  ${D}/usr/lib/libql_utils.so.1 ${D}/usr/lib/libql_utils.so
      ln -sf -r  ${D}/usr/lib/libql_mgmt_client.so.1 ${D}/usr/lib/libql_mgmt_client.so
      ln -sf -r  ${D}/usr/lib/libql_mam_ipc.so.1 ${D}/usr/lib/libql_mam_ipc.so
      ln -sf -r  ${D}/usr/lib/libdsi_netctrl.so.0 ${D}/usr/lib/libdsi_netctrl.so
      ln -sf -r  ${D}/usr/lib/libql_usb.so.1 ${D}/usr/lib/libql_usb.so
      ln -sf -r  ${D}/usr/lib/libqcmapipc.so.1 ${D}/usr/lib/libqcmapipc.so
      ln -sf -r  ${D}/usr/lib/libql_tts_eng.so.1 ${D}/usr/lib/libql_tts.so.1
}
