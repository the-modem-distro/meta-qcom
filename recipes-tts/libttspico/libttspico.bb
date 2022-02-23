DESCRIPTION = "SVOX Pico Library"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/\
${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"
PROVIDES = "libttspico"
MY_PN = "libttspico"
PR = "r0"
S = "${WORKDIR}"

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
      file://picowa.c"

CCFILES = "picoacph.c \
	picoapi.c \
	picobase.c \
	picocep.c \
	picoctrl.c \
	picodata.c \
	picodbg.c \
	picoextapi.c \
	picofftsg.c \
	picokdbg.c \
	picokdt.c \
	picokfst.c \
	picoklex.c \
	picoknow.c \
	picokpdf.c \
	picokpr.c \
	picoktab.c \
	picoos.c \
	picopal.c \
	picopam.c \
	picopr.c \
	picorsrc.c \
	picosa.c \
	picosig.c \
	picosig2.c \
	picospho.c \
	picotok.c \
	picotrns.c \
	picowa.c"
S = "${WORKDIR}"

do_compile() {
    ${CC} ${LDFLAGS} -shared -fpic -O2 picoacph.c  picoapi.c  picobase.c  picocep.c  picoctrl.c  picodata.c  picodbg.c  picoextapi.c  picofftsg.c  picokdbg.c  picokdt.c  picokfst.c  picoklex.c  picoknow.c  picokpdf.c  picokpr.c  picoktab.c  picoos.c  picopal.c  picopam.c  picopr.c  picorsrc.c  picosa.c  picosig.c  picosig2.c  picospho.c  picotok.c  picotrns.c  picowa.c -o libttspico.so.0 -lm
}

do_install() {
      install -d ${D}/usr/lib
      install -m 0755  ${S}/libttspico.so.0 ${D}/usr/lib/
}

do_install:append() {
      ln -s ${D}/usr/lib/libttspico.so.0 ${D}/usr/lib/libttspico.so
}