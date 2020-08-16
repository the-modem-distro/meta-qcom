# Recipe for building the MDM9607 Kernel tree
# Biktorgj 2020
# Released under the MIT license (see COPYING.MIT for the terms)

inherit kernel

DESCRIPTION = "Linux Kernel for Qualcomm MDM9607-MTP"
LICENSE = "GPLv2"

# Set compatible machines for this kernel
COMPATIBLE_MACHINE = "(mdm9607|apq8009|apq8096|apq8053|msm8909w)"
# Dependencies: skales-native (provides mkbootimg,dtbtool), gcc
DEPENDS += "skales-native libgcc"


# Base paths
SRC_DIR   =  "/kernel/msm-3.18"
S         =  "${WORKDIR}/kernel/msm-3.18"
GITVER    =  "${@base_get_metadata_git_revision('${SRC_DIR}',d)}"
PV = "git"
PR = "r5-${DISTRO}"

KERNEL_IMAGETYPE ?= "zImage"
KERNEL_DEFCONFIG_mdm9607 ?= "${S}/arch/arm/configs/mdm9607-perf_defconfig"

QCOM_BOOTIMG_ROOTFS ?= "undefined"
SD_QCOM_BOOTIMG_ROOTFS ?= "undefined"

# set output file names
BOOT_IMAGE_BASE_NAME = "boot-${KERNEL_IMAGE_NAME}"
BOOT_IMAGE_SYMLINK_NAME = "boot-${KERNEL_IMAGE_LINK_NAME}"
# CMDLine params
KERNEL_CMDLINE = "root=rw rootwait console=ttyHSL0,115200n8"


kernel_conf_variable() {
	CONF_SED_SCRIPT="$CONF_SED_SCRIPT /CONFIG_$1[ =]/d;"
	if test "$2" = "n"
	then
		echo "# CONFIG_$1 is not set" >> ${B}/.config
	else
		echo "CONFIG_$1=$2" >> ${B}/.config
	fi
}


do_configure_prepend() {
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

    # Check for kernel config fragments.  The assumption is that the config
    # fragment will be specified with the absolute path.  For example:
    #   * ${WORKDIR}/config1.cfg
    #   * ${S}/config2.cfg
    # Iterate through the list of configs and make sure that you can find
    # each one.  If not then error out.
    # NOTE: If you want to override a configuration that is kept in the kernel
    #       with one from the OE meta data then you should make sure that the
    #       OE meta data version (i.e. ${WORKDIR}/config1.cfg) is listed
    #       after the in kernel configuration fragment.
    # Check if any config fragments are specified.
    if [ ! -z "${KERNEL_CONFIG_FRAGMENTS}" ]
    then
        for f in ${KERNEL_CONFIG_FRAGMENTS}
        do
            # Check if the config fragment was copied into the WORKDIR from
            # the OE meta data
            if [ ! -e "$f" ]
            then
                echo "Could not find kernel config fragment $f"
                exit 1
            fi
        done

        # Now that all the fragments are located merge them.
        ( cd ${WORKDIR} && ${S}/scripts/kconfig/merge_config.sh -m -r -O ${B} ${B}/.config ${KERNEL_CONFIG_FRAGMENTS} 1>&2 )
    fi

	yes '' | oe_runmake -C ${S} O=${B} oldconfig
        oe_runmake -C ${S} O=${B} savedefconfig && cp ${B}/defconfig ${WORKDIR}/defconfig.saved
}

# append DTB
do_compile_append() {
    if ! [ -e ${B}/arch/${ARCH}/boot/dts/${KERNEL_DEVICETREE} ] ; then
        oe_runmake ${KERNEL_DEVICETREE}
    fi
    cp arch/${ARCH}/boot/${KERNEL_IMAGETYPE} arch/${ARCH}/boot/${KERNEL_IMAGETYPE}.backup
    cat arch/${ARCH}/boot/${KERNEL_IMAGETYPE}.backup arch/${ARCH}/boot/dts/${KERNEL_DEVICETREE} > arch/${ARCH}/boot/${KERNEL_IMAGETYPE}
    rm -f arch/${ARCH}/boot/${KERNEL_IMAGETYPE}.backup
}


# param ${1} partition where rootfs is located
# param ${2} output boot image file name
priv_make_image() {
    ${STAGING_BINDIR_NATIVE}/skales/mkbootimg --kernel ${B}/arch/${ARCH}/boot/${KERNEL_IMAGETYPE} \
              --ramdisk ${B}/initrd.img \
              --output ${DEPLOYDIR}/${2}.img \
              --pagesize ${QCOM_BOOTIMG_PAGE_SIZE} \
              --base ${QCOM_BOOTIMG_KERNEL_BASE} \
              --cmdline "${KERNEL_CMDLINE}"
}

do_deploy_append() {
    tmp="ttyHSL0"
    baudrate="115200n8"
    ttydev="ttyHSL0"

    # mkbootimg requires an initrd file, make fake one that will be ignored
    # during boot
    echo "This is not an initrd" > ${B}/initrd.img

    # don't build bootimg if rootfs partition is not defined
    if [ "${QCOM_BOOTIMG_ROOTFS}" == "undefined"]; then
        bbfatal "Rootfs partition must be defined"
    fi

    priv_make_image ${QCOM_BOOTIMG_ROOTFS} ${BOOT_IMAGE_BASE_NAME}
    ln -sf ${BOOT_IMAGE_BASE_NAME}.img ${DEPLOYDIR}/${BOOT_IMAGE_SYMLINK_NAME}.img
}




