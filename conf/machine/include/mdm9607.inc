SOC_FAMILY = "mdm9607"
require conf/machine/include/soc-family.inc

PREFERRED_PROVIDER_virtual/kernel ?= "linux-linaro-qcomlt"

MACHINE_EXTRA_RRECOMMENDS += " \
    fastrpc \
"

QCOM_BOOTIMG_KERNEL_BASE ?= "0x80000000"
QCOM_BOOTIMG_PAGE_SIZE ?= "4096"

# Uncomment the following line to enable the hard floating point abi. Note that
# this breaks some binary libraries and 3D (neither of which ship with
# meta-yocto). For maximum compatibility, leave this disabled.
#DEFAULTTUNE ?= "cortexa8hf-neon"

require conf/machine/include/tune-cortexa8.inc

BBMASK = ".*/*/recipes-kernel/linux-msm-3.18"


SERIAL_CONSOLE ?= "115200 ttyHSL0"

PAGE_SIZE ?= '4096'

# Max supported inodes
EXTRA_IMAGECMD ?= "-N 2048"

#UBINIZE Args
UBINIZE_ARGS ?= "-m 4096 -p 256KiB -s 4096"
#MKUBIFS Arg
MKUBIFS_ARGS ?= "-m 4096 -e 253952 -c 2146 -F"
#Volume sizes
SYSTEM_VOLUME_SIZE ?= "63MiB"
CACHE_VOLUME_SIZE ?= "55MiB"
