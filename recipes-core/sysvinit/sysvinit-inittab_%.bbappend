# ATFWD Daemon requires obsolete libcrypt libraries
# With this I remove the --disable-obsolete-api flag from libxcrypt
# With this and after patching the ELF so it looks for the file in the
# correct path, the daemon can start

do_install_append() {
  echo "m2:5:respawn:/usr/bin/mbimd" >> ${D}/etc/inittab
  echo "m3:5:respawn:/usr/bin/diagrebootapp" >> ${D}/etc/inittab
}