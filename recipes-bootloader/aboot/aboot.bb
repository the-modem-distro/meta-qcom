# Recipe for building LK

inherit deploy

DESCRIPTION = "LK2ND: Little Kernel Bootloader for Qcom SoCs"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/\
${LICENSE};md5=0835ade698e0bcf8506ecda2f7b4f302"
RPROVIDES_${PN} = "aboot"

# We tell yocto what does this provide
PROVIDES = "virtual/bootloader"

# Set the project name.. can't use ${MACHINE} ?
ABOOT_PROJECT:mdm9607 = "mdm9607"
ABOOT_PROJECT:mdm9640 = "mdm9640"
ABOOT_PROJECT:msm8916 = "msm8916"

# Dependencies
DEPENDS += "libgcc" 

# Base paths
SRC_URI   =  "git://github.com/Biktorgj/lk2nd.git;protocol=https;branch=quectel-eg25-timer"
SRCREV = "${AUTOREV}"
S = "${WORKDIR}/git"
PV = "1.0+git-${SRCPV}"

# Force bitbake to always rebuild and reinstall
# Otherwise unless you really clean everything it won't automatically
# build it again until there's a change in the repo
do_compile[nostamp] = "1"
do_install[nostamp] = "1"

# I need to find a better way for this or it will break on next release
LIBGCC = "${STAGING_LIBDIR}/${TARGET_SYS}/11.3.0/libgcc.a"

# Project options for this board
LKSETTINGS = "SIGNED_KERNEL=0 DEBUG=1 ENABLE_DISPLAY=0 WITH_DEBUG_UART=1 BOARD=${ABOOT_BOARD} SMD_SUPPORT=1 MMC_SDHCI_SUPPORT=0 FASTBOOT_TIMER=1"

# What actually gets run
EXTRA_OEMAKE = "${ABOOT_PROJECT} -C ${S} LIBGCC='${LIBGCC}' ENABLE_HARD_FPU=1 TOOLCHAIN_PREFIX=${TARGET_PREFIX} ${LKSETTINGS}"

do_install() {
	install -d ${DEPLOY_DIR_IMAGE}
	install ${B}/build-${ABOOT_PROJECT}/appsboot.mbn ${DEPLOY_DIR_IMAGE}/
}