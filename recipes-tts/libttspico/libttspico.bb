DESCRIPTION = "SVOX Pico Library"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/\
${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"
MY_PN = "libttspico-dev"
PROVIDES ="libttspico"
PROVIDES ="libttspico-dev"
# Unless we specifically state this, binaries using the library will fail QA
RPROVIDES:${PN} ="libttspico.so"
DEPENDS = "popt"
PR = "r0"
S = "${WORKDIR}"
INSANE_SKIP:${PN} += "dev-so"
FILES:${PN} += "/usr/share/lang/* \
                /usr/lib/*.so*"

INSANE_SKIP_${PN} += " ldflags"
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_SYSROOT_STRIP = "1"
SOLIBS = ".so*"
FILES_SOLIBSDEV = ""

SRC_URI="file://picobase.c \
      file://picodbg.c \
      file://picokdbg.c \
      file://picoknow.c \
      file://picoos.c \
      file://picopr.h \
      file://picosig.h \
      file://picowa.h \
      file://picobase.h \
      file://picodbg.h \
      file://picokdbg.h \
      file://picoknow.h \
      file://picoos.h \
      file://picorsrc.c \
      file://picospho.c \
      file://picocep.c \
      file://picodefs.h \
      file://picokdt.c \
      file://picokpdf.c \
      file://picopal.c \
      file://picorsrc.h \
      file://picospho.h \
      file://picoacph.c \
      file://picocep.h \
      file://picodsp.h \
      file://picokdt.h \
      file://picokpdf.h \
      file://picopal.h \
      file://picosa.c \
      file://picotok.c \
      file://picoacph.h \
      file://picoctrl.c \
      file://picoextapi.c \
      file://picokfst.c \
      file://picokpr.c \
      file://picopam.c \
      file://picosa.h \
      file://picotok.h \
      file://picoapi.c \
      file://picoctrl.h \
      file://picoextapi.h \
      file://picokfst.h \
      file://picokpr.h \
      file://picopam.h \
      file://picosig2.c \
      file://picotrns.c \
      file://picoapid.h \
      file://picodata.c \
      file://picofftsg.c \
      file://picoklex.c \
      file://picoktab.c \
      file://picopltf.h \
      file://picosig2.h \
      file://picotrns.h \
      file://picoapi.h \
      file://picodata.h \
      file://picofftsg.h \
      file://picoklex.h \
      file://picoktab.h \
      file://picopr.c \
      file://picosig.c \
      file://picowa.c \
      file://lang/de-DE_gl0_sg.bin \
      file://lang/en-GB_ta.bin \
      file://lang/es-ES_ta.bin \
      file://lang/fr-FR_ta.bin \
      file://lang/de-DE_ta.bin \
      file://lang/en-US_lh0_sg.bin \
      file://lang/es-ES_zl0_sg.bin \
      file://lang/it-IT_cm0_sg.bin \
      file://lang/en-GB_kh0_sg.bin \
      file://lang/en-US_ta.bin \
      file://lang/fr-FR_nk0_sg.bin \
      file://lang/it-IT_ta.bin"

do_compile() {
    ${CC} ${LDFLAGS} -shared -fpic -O2 picoacph.c  picoapi.c  picobase.c  picocep.c  picoctrl.c  picodata.c  picodbg.c  picoextapi.c  picofftsg.c  picokdbg.c  picokdt.c  picokfst.c  picoklex.c  picoknow.c  picokpdf.c  picokpr.c  picoktab.c  picoos.c  picopal.c  picopam.c  picopr.c  picorsrc.c  picosa.c  picosig.c  picosig2.c  picospho.c  picotok.c  picotrns.c  picowa.c -o libttspico.so.0 -lm
}

do_install() {
      install -d ${D}/usr/share/lang/
      install -d ${D}/usr/lib
      install -d ${D}${includedir}

      install -m 0755  ${S}/libttspico.so.0 ${D}${libdir}/
      install -m 0644 ${S}/lang/en-US_lh0_sg.bin ${D}/usr/share/lang/
      install -m 0644 ${S}/lang/en-US_ta.bin ${D}/usr/share/lang/

# Other language definitions not needed here
   #   install -m 0644 ${S}/lang/en-GB_kh0_sg.bin ${D}/usr/share/lang/
   #   install -m 0644 ${S}/lang/en-GB_ta.bin ${D}/usr/share/lang/
   #   install -m 0644 ${S}/lang/de-DE_gl0_sg.bin ${D}/usr/share/lang/
   #   install -m 0644 ${S}/lang/es-ES_ta.bin ${D}/usr/share/lang/
   #   install -m 0644 ${S}/lang/fr-FR_ta.bin ${D}/usr/share/lang/
   #   install -m 0644 ${S}/lang/de-DE_ta.bin ${D}/usr/share/lang/
   #   install -m 0644 ${S}/lang/es-ES_zl0_sg.bin ${D}/usr/share/lang/
   #   install -m 0644 ${S}/lang/it-IT_cm0_sg.bin ${D}/usr/share/lang/
   #   install -m 0644 ${S}/lang/fr-FR_nk0_sg.bin ${D}/usr/share/lang/
   #   install -m 0644 ${S}/lang/it-IT_ta.bin ${D}/usr/share/lang/

# Copy headers into bitbake's include directory so other binaries can have them
      install -m 0755 ${WORKDIR}/picoapi.h  ${D}${includedir}/
      install -m 0755 ${WORKDIR}/picoapid.h  ${D}${includedir}/
      install -m 0755 ${WORKDIR}/picoos.h  ${D}${includedir}/
      install -m 0755 ${WORKDIR}/picodefs.h  ${D}${includedir}/
      install -m 0755 ${WORKDIR}/picopal.h  ${D}${includedir}/
      install -m 0755 ${WORKDIR}/picopltf.h  ${D}${includedir}/
      install -m 0755 ${WORKDIR}/picoknow.h  ${D}${includedir}/
      install -m 0755 ${WORKDIR}/picorsrc.h  ${D}${includedir}/
      install -m 0755 ${WORKDIR}/picoctrl.h  ${D}${includedir}/
      install -m 0755 ${WORKDIR}/picodata.h  ${D}${includedir}/
      install -m 0755 ${WORKDIR}/picotrns.h  ${D}${includedir}/
      install -m 0755 ${WORKDIR}/picokfst.h  ${D}${includedir}/
      install -m 0755 ${WORKDIR}/picodbg.h  ${D}${includedir}/
      install -m 0755 ${WORKDIR}/picoktab.h  ${D}${includedir}/

# We need the symlink so other recipes can find the library
      ln -sf -r ${D}/usr/lib/libttspico.so.0 ${D}/usr/lib//libttspico.so
}
