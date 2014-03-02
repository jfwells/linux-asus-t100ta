ifeq ($(KERNELRELEASE),)  

KERNELDIR ?= /lib/modules/$(shell uname -r)/build 
PWD := $(shell pwd)  

.PHONY: build clean  

build:
	
	$(MAKE) -C $(KERNELDIR) M=$(PWD)  modules  
	@echo "\n\n**************************\n"
	@echo "Module compiled successfully.\n\nTo install it, run 'make install', as root.\n"
	@echo "******************************\n"
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c *.symvers *.order

install:
	cp cm3218.ko /lib/modules/`uname -r`/kernel/drivers/iio/light
	depmod
	@echo "\n\n**************************\n"
	@echo "Module installed for this kernel.\n\nRun it with 'modprobe cm3218' as root.\n"	
	@echo "******************************\n"
else  

$(info Building with KERNELRELEASE = ${KERNELRELEASE}) 

ccflags-y := -std=gnu99 -Wno-declaration-after-statement
obj-m :=    cm3218.o  

endif
