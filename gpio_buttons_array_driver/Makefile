ifeq ($(KERNELRELEASE),)  

KERNELDIR ?= /lib/modules/$(shell uname -r)/build 
PWD := $(shell pwd)  

.PHONY: build clean  

build:
	
	$(MAKE) -C $(KERNELDIR) M=$(PWD)  modules  

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c 
else  

$(info Building with KERNELRELEASE = ${KERNELRELEASE}) 

ccflags-y := -std=gnu99 -Wno-declaration-after-statement
obj-m :=    gpio_button_array.o  

endif
