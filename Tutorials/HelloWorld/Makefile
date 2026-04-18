obj-m += hello_world.o

KDIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)
SIGN_FILE = $(KDIR)/scripts/sign-file
KEY = $(HOME)/module-signing/MOK.key
CERT = $(HOME)/module-signing/MOK.crt

all:
	make -C $(KDIR) M=$(shell pwd) modules
	$(SIGN_FILE) sha256 $(KEY) $(CERT) hello_world.ko
clean:
	make -C $(KDIR) M=$(shell pwd) clean