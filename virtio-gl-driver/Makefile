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

i insmod:
	sudo dmesg -C
	sudo rmmod $(MOD)
	sudo insmod ${MOD}.ko
# -sudo chmod 666 /dev/gl

r rmmod:
	@if [ -n "`lsmod | grep $(MOD)`" ]; then \
	sudo rmmod $(MOD);  \
	echo "sudo rmmod $(MOD)"; \
	fi

s show_dmesg:
	@echo "============================================================="
	-@dmesg
	@echo "============================================================="

clean:
	rm -rf *.o *.ko *.mod.c *.order *.symvers .*.cmd .tmp_versions
.PHONY:clean


# MOD	:= virtio_gl

# # KBUILD_CFLAGS   := -Wno-error
# # KBUILD_CFLAGS   := -fno-common 
# # -mcmodel=kernel

# obj-m := ${MOD}_driver.o
# # ${MOD}-objs := ${MOD}_driver.o

# #CFLAGS_MODULE+=-I/usr/local/cuda/include
# KERDIR:=/lib/modules/$(shell uname -r)/build
# PWD=$(shell pwd)

# all: c

# c compile:
# 	make -C $(KERDIR) SUBDIRS=$(PWD) modules

# clean:
# 	make -C $(KERDIR) M=$(PWD) clean
# 	rm -f modules.order *.o *.mod.c *.mod.o *.ko



