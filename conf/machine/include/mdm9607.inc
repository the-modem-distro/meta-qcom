SOC_FAMILY = "mdm9607"
require conf/machine/include/soc-family.inc
require conf/machine/include/tune-cortexa8.inc

PREFERRED_PROVIDER_virtual/kernel ?= "linuxmdm"

MACHINE_EXTRA_RRECOMMENDS += " \
    fastrpc \
"

QCOM_BOOTIMG_KERNEL_BASE ?= "0x80000000"
QCOM_BOOTIMG_PAGE_SIZE ?= "2048"

# Uncomment the following line to enable the hard floating point abi. Note that
# this breaks some binary libraries and 3D (neither of which ship with
# meta-yocto). For maximum compatibility, leave this disabled.
#DEFAULTTUNE ?= "cortexa8hf-neon"

# Image Generation
# Make ubi filesystem and then pack it in a ubi container, as stock
IMAGE_FSTYPES ?= "ubi multiubi "

# Set the volume name inside the ubi image
UBI_VOLNAME = "rootfs"
ROOT_FLASH_SIZE = "63"
SYSTEM_VOLUME_SIZE ?= "63MiB"
CACHE_VOLUME_SIZE ?= "55MiB"

# Max supported inodes
EXTRA_IMAGECMD ?= "-N 2048"

MKUBIFS_ARGS="-m 2048 -e 126976 -c 4292 -F"
UBINIZE_ARGS="-m 2048 -p 128KiB -s 2048"