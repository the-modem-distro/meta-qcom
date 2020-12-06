# ATFWD Daemon requires obsolete libcrypt libraries
# With this I remove the --disable-obsolete-api flag from libxcrypt
# With this and after patching the ELF so it looks for the file in the
# correct path, the daemon can start

# The modem works stable, even without this enabled
# As long as the binaries are there
# So keep them and set the init as commented just in case
# we need it for anything later
do_install_append() {
  # echo "#m2:5:respawn:/usr/bin/mbimd" >> ${D}/etc/inittab
  # echo "#m3:5:respawn:/usr/bin/diagrebootapp" >> ${D}/etc/inittab
}