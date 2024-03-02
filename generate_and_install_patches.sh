XILINX_PROJ_DIR=${1:-"/home/myles/rdf0428-zcu106-vcu-trd-2020.1/apu/vcu_petalinux_bsp/vronicam_plinux"}

LIBRARIES_SOURCE_DIR="./"

echo "installing all patches to project folder: $XILINX_PROJ_DIR";

# vcu-modules
git diff vcu-modules-diff..main -- vcu-modules/ ':!*.gitignore' ':(exclude)vcu-modules/.vscode' > vcu-modules.patch;
sed 's/vcu-modules\///g' vcu-modules.patch > vcu-modules-new.patch
cp vcu-modules-new.patch $XILINX_PROJ_DIR/project-spec/meta-user/recipes-multimedia/vcu/kernel-module-vcu/0001-vcu-modules.patch;
# rm vcu-modules.patch vcu-modules-new.patch

# vcu-omx-il
git diff vcu-modules-diff..main -- vcu-omx-il/ ':!*.gitignore' ':(exclude)vcu-modules/.vscode' > vcu-omx-il.patch;
sed 's/vcu-omx-il\///g' vcu-omx-il.patch > vcu-omx-il-new.patch
cp vcu-omx-il-new.patch $XILINX_PROJ_DIR/project-spec/meta-user/recipes-multimedia/vcu/libomxil-xlnx/0001-vcu-omx-il.patch;
# rm vcu-omx-il.patch vcu-omx-il-new.patch

# vcu-ctrl-sw
git diff vcu-modules-diff..main -- vcu-ctrl-sw/ ':!*.gitignore' ':(exclude)vcu-modules/.vscode' > vcu-ctrl-sw.patch;
sed 's/vcu-ctrl-sw\///g' vcu-ctrl-sw.patch > vcu-ctrl-sw-new.patch
cp vcu-ctrl-sw-new.patch $XILINX_PROJ_DIR/project-spec/meta-user/recipes-multimedia/vcu/libvcu-xlnx/0001-vcu-ctrl-sw.patch;
# rm vcu-ctrl-sw.patch vcu-ctrl-sw-new.patch



