# ATFWD Daemon requires obsolete libcrypt libraries
# With this I remove the --disable-obsolete-api flag from libxcrypt
# With this and after patching the ELF so it looks for the file in the
# correct path, the daemon can start

API = ""
EXTRA_OECONF += "${API}"