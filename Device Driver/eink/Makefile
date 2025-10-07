obj-m += mikroe_eink.o
mikroe_eink-objs := eink_spi.o eink.o

# Kernel build variables
KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	dtc -@ -I dts -O dtb -o eink_overlay.dtbo eink_overlay.dts 
	
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf eink_overlay.dtbo

