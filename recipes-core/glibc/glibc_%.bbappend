# Quectel's proprietary files require ld-linux.so.3 to live
# in /lib, but Yocto's glibc recipe leaves is as ld-linus-armhf.so.3
# and doesn't make a symlink, so we do it ourselves 
FILES_${PN} += "/lib/ld-linux-armhf.so.3"

RPROVIDES_${PN} += "ld-linux.so.3"
do_install_append() {
        install -d ${D}/lib
        ln -sf -r  ${D}/lib/ld-linux-armhf.so.3 ${D}/lib/ld-linux.so.3
    #    cd ${D}/lib
    #    ln -s ld-linux-armhf.so.3 ld-linux.so.3
}