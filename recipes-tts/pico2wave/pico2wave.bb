DESCRIPTION = "SVOX Pico2Wave"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/\
${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"
MY_PN = "pico2wave"
PROVIDES ="pico2wave"
# Unless we specifically state this, binaries using the library will fail QA
DEPENDS+="libttspico popt"
RDEPENDS:${PN} = "libttspico"
PR = "r0"
S = "${WORKDIR}"
SRC_URI="file://pico2wave.c"

do_compile() {
   ${CC} ${LDFLAGS} -O2 pico2wave.c -o pico2wave -lpopt -lttspico
}

do_install() {
      install -d ${D}/usr/bin
      install -m 0755  ${S}/pico2wave ${D}/usr/bin/
}
