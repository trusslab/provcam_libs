XILINX_PROJ_DIR=${1:-"/home/myles/rdf0428-zcu106-vcu-trd-2020.1/apu/vcu_petalinux_bsp/xilinx-vcu-zcu106-v2020.1-final"}

LIBRARIES_SOURCE_DIR="./"

echo "installing all patches to project folder: $XILINX_PROJ_DIR";

# vcu-modules
cd $LIBRARIES_SOURCE_DIR/vcu-modules;
git diff > vcu-modules.patch;
cp vcu-modules.patch $XILINX_PROJ_DIR/project-spec/meta-user/recipes-multimedia/vcu/kernel-module-vcu/0001-vcu-modules.patch;

# vcu-omx-il
cd ../vcu-omx-il;
git diff > vcu-omx-il.patch;
cp vcu-omx-il.patch $XILINX_PROJ_DIR/project-spec/meta-user/recipes-multimedia/vcu/libomxil-xlnx/0001-vcu-omx-il.patch;

# vcu-ctrl-sw
cd ../vcu-ctrl-sw;
git diff > vcu-ctrl-sw.patch;
cp vcu-ctrl-sw.patch $XILINX_PROJ_DIR/project-spec/meta-user/recipes-multimedia/vcu/libvcu-xlnx/0001-vcu-ctrl-sw.patch;



