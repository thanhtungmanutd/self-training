obj-m += pocket_geiger.o

# Kernel build variables
KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	dtc -@ -I dts -O dtb -o pocket_geiger_overlay.dtbo pocket_geiger_overlay.dts 

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf pocket_geiger_overlay.dtbo

