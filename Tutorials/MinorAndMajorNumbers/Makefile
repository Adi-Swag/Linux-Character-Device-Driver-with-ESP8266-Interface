obj-m += static_allocation.o
obj-m += dynamic_allocation.o

KDIR = /lib/modules/$(shell uname -r)/build
SIGN_FILE = $(KDIR)/scripts/sign-file
KEY = $(HOME)/module-signing/MOK.key
CERT = $(HOME)/module-signing/MOK.crt

all:
	make -C $(KDIR) M=$(shell pwd) modules
	$(SIGN_FILE) sha256 $(KEY) $(CERT) static_allocation.ko
	$(SIGN_FILE) sha256 $(KEY) $(CERT) dynamic_allocation.ko
clean:
	make -C $(KDIR) M=$(shell pwd) clean
