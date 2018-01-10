#

ifneq ($(KERNELRELEASE),)
	obj-m := fb4qt.o
else
	KERNELDIR = /opt/brcm7252/linux-3.14-1.6/stblinux-3.14-1.6/
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=/opt/toolchains/stbgcc-4.8-1.4/bin/arm-linux-
endif

clean:
	rm -rf .*.cmd *.o *.mod.c *.ko .tmp_versions
