SRC := $(shell pwd)

ifeq ($(O),)
	include $(KERNEL_SRC)/.config
else
	include $(O)/.config
endif

obj-m	+= common/
# obj-$(CONFIG_ALLEGRO_SHIROHA)	+= common/
obj-m	+= al5e/
# obj-$(CONFIG_ALLEGRO_VCU_ENC_DRIVER_SHIROHA) += al5e/
obj-m	+= al5d/
# obj-$(CONFIG_ALLEGRO_VCU_DEC_DRIVER_SHIROHA) += al5d/
obj-m	+= dmaproxy/
# obj-$(CONFIG_DMAPROXY_SHIROHA) += dmaproxy/

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) O=$(O) modules

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order modules.builtin
	rm -f */*.ko */*.mod.c */.*.mod.c */.*.cmd */*.o
	rm -f */modules.order */modules.builtin
	rm -rf .tmp_versions Modules.symvers
