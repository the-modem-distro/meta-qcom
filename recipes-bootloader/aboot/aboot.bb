# Recipe for building the MDM9607 Kernel tree
# Biktorgj 2022
# Released under the MIT license (see COPYING.MIT for the terms)

inherit deploy
DESCRIPTION = "LK2ND: Little Kernel Bootloader for Qcom SoCs"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/\
${LICENSE};md5=0835ade698e0bcf8506ecda2f7b4f302"
# We tell yocto what does this provide
PROVIDES = "aboot"
PROVIDES+= "virtual/bootloader"

# Set the project name
ABOOT_PROJECT = "mdm9607"
# Dependencies
DEPENDS += "libgcc" 

# Base paths
SRC_URI   =  "git://github.com/Biktorgj/lk2nd.git;protocol=https;branch=fastboot_timer_test"
SRCREV = "${AUTOREV}"
S = "${WORKDIR}/git"

PR = "${DISTRO}"
PVBASE := "${PV}"
PV = "1.0+git${SRCPV}"

# I need to find a better way for this or it will break on next release
LIBGCC = "${STAGING_LIBDIR}/${TARGET_SYS}/11.2.0/libgcc.a"

# Project options for this board
LKSETTINGS = "SIGNED_KERNEL=0 DEBUG=1 ENABLE_DISPLAY=0 WITH_DEBUG_UART=1 BOARD=9607 SMD_SUPPORT=1 MMC_SDHCI_SUPPORT=0"

# What actually gets run
EXTRA_OEMAKE = "${ABOOT_PROJECT} -C ${S} O=${B} LIBGCC='${LIBGCC}' ENABLE_HARD_FPU=1 TOOLCHAIN_PREFIX=${TARGET_PREFIX} ${LKSETTINGS}"

do_install() {
	install -d ${DEPLOY_DIR_IMAGE}
	install ${S}/build-${ABOOT_PROJECT}/appsboot.mbn ${DEPLOY_DIR_IMAGE}/
}