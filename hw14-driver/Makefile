# Makefile для компиляции драйвера мыши

obj-m += mouse_driv.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	sudo depmod -a

uninstall:
	sudo rm -f /lib/modules/$(shell uname -r)/extra/mouse_driv.ko
	sudo depmod -a