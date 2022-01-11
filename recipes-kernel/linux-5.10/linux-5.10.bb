# Recipe for building the MDM9607 Kernel tree
# Biktorgj 2020
# Released under the MIT license (see COPYING.MIT for the terms)

inherit kernel
inherit pythonnative

DESCRIPTION = "Linux Kernel for Qualcomm MDM9607-MTP"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

# Set compatible machines for this kernel
COMPATIBLE_MACHINE = "mdm9607"
# Dependencies
DEPENDS += "mkbootimg-native dtbtool-native libgcc python-native dtc-native"

# Base paths
SRC_URI   =  "git://github.com/SoMainline/linux.git;protocol=https;branch=konrad/pinemodem"
SRCREV = "${AUTOREV}"
S = "${WORKDIR}/git"

PR = "${DISTRO}"
PVBASE := "${PV}"
PV = "1.0+git${SRCPV}"
KERNEL_IMAGETYPE ?= "zImage"
KERNEL_DEFCONFIG_mdm9607 ?= "${S}/arch/arm/configs/qcom_defconfig"
KERNEL_DEVICETREE ?= "qcom-mdm9607-quectel-eg25.dtb"

QCOM_BOOTIMG_ROOTFS ?= "undefined"
SD_QCOM_BOOTIMG_ROOTFS ?= "undefined"
# set output file names
BOOT_IMAGE_BASE_NAME = "boot-${KERNEL_IMAGE_NAME}"
BOOT_IMAGE_SYMLINK_NAME = "boot-${KERNEL_IMAGE_LINK_NAME}"
# CMDLine params
QCOM_BOOTIMG_PAGE_SIZE = "2048"
KERNEL_CMDLINE = "noinitrd ro console=ttyHSL0,115200,n8 androidboot.hardware=qcom ehci-hcd.park=3 msm_rtb.filter=0x37 lpm_levels.sleep_disabled=1 earlycon=msm_hsl_uart,0x78b3000"
QCOM_BOOTIMG_KERNEL_BASE = "0x80000000"
KERNEL_TAGS_ADDR = "0x81E00000"
do_compile[nostamp] = "1"

kernel_conf_variable() {
	CONF_SED_SCRIPT="$CONF_SED_SCRIPT /CONFIG_$1[ =]/d;"
	if test "$2" = "n"
	then
		echo "# CONFIG_$1 is not set" >> ${B}/.config
	else
		echo "CONFIG_$1=$2" >> ${B}/.config
	fi
}


do_configure:prepend() {
	echo "" > ${B}/.config
	CONF_SED_SCRIPT=""

	kernel_conf_variable LOCALVERSION "\"${LOCALVERSION}\""
	kernel_conf_variable LOCALVERSION_AUTO y

	if [ -f '${WORKDIR}/defconfig' ]; then
		sed -e "${CONF_SED_SCRIPT}" < '${WORKDIR}/defconfig' >> '${B}/.config'
	else
		sed -e "${CONF_SED_SCRIPT}" < '${KERNEL_DEFCONFIG}' >> '${B}/.config'
	fi

	if [ "${SCMVERSION}" = "y" ]; then
		# Add GIT revision to the local version
		head=`git --git-dir=${S}/.git  rev-parse --verify --short HEAD 2> /dev/null`
		printf "%s%s" +g $head > ${B}/.scmversion
	fi

	yes '' | oe_runmake -C ${S} O=${B} oldconfig
        oe_runmake -C ${S} O=${B} savedefconfig && cp ${B}/defconfig ${WORKDIR}/defconfig.saved
}

# append DTB
do_compile:append() {
    ${STAGING_BINDIR_NATIVE}/dtbTool ${B}/arch/arm/boot/dts/ -s 2048 -o ${B}/arch/arm/boot/dts/dtb.img -p scripts/dtc/ -v
}


# param ${1} partition where rootfs is located
# param ${2} output boot image file name
priv_make_image() {
    ${STAGING_BINDIR_NATIVE}/mkbootimg --kernel ${B}/arch/arm/boot/${KERNEL_IMAGETYPE} \
              --ramdisk ${B}/initrd.gz \
              --output ${DEPLOYDIR}/${2}.img \
              --pagesize ${QCOM_BOOTIMG_PAGE_SIZE} \
              --base ${QCOM_BOOTIMG_KERNEL_BASE} \
              --tags-addr ${KERNEL_TAGS_ADDR} \
              --dt ${B}/arch/arm/boot/dts/dtb.img \
              --cmdline "${KERNEL_CMDLINE}"
}

do_deploy:append() {
    # mkbootimg requires an initrd file, make fake one that will be ignored
    # during boot
    touch ${B}/initrd.gz

    # don't build bootimg if rootfs partition is not defined
    if [ "${QCOM_BOOTIMG_ROOTFS}" == "undefined"]; then
        bbfatal "Rootfs partition must be defined"
    fi

    priv_make_image ${QCOM_BOOTIMG_ROOTFS} ${BOOT_IMAGE_BASE_NAME}
    ln -sf ${BOOT_IMAGE_BASE_NAME}.img ${DEPLOYDIR}/${BOOT_IMAGE_SYMLINK_NAME}.img
}
