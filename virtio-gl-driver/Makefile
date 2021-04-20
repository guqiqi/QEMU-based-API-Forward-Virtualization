# Makefile 2.6

MOD	:= virtio_gl_driver

ifneq	($(KERNELRELEASE),)
obj-m := ${MOD}.o
else
KERNEL_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
all:
	make -C $(KERNEL_DIR) M=$(PWD) modules
endif

i insmod: c all
	sudo dmesg -C
	@if [ -n "`lsmod | grep $(MOD)`" ]; then \
	sudo rmmod $(MOD); \
	fi
	sudo insmod ${MOD}.ko
	-sudo chmod 666 /dev/virtio-gl

r rmmod:
	@if [ -n "`lsmod | grep $(MOD)`" ]; then \
	sudo rmmod $(MOD);  \
	echo "sudo rmmod $(MOD)"; \
	fi

s show_dmesg:
	@echo "============================================================="
	-@dmesg
	@echo "============================================================="

c clean:
	rm -rf *.o *.ko *.mod.c *.order *.symvers .*.cmd .tmp_versions
.PHONY:clean




