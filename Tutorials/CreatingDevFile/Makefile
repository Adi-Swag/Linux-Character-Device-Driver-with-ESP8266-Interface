obj-m += manual_creation.o
obj-m += automatic_creation.o

KDIR = /lib/modules/$(shell uname -r)/build
SIGN_FILE = $(KDIR)/scripts/sign-file
KEY = $(HOME)/module-signing/MOK.key
CERT = $(HOME)/module-signing/MOK.crt

all:
	make -C $(KDIR) M=$(shell pwd) modules
	$(SIGN_FILE) sha256 $(KEY) $(CERT) manual_creation.ko
	$(SIGN_FILE) sha256 $(KEY) $(CERT) automatic_creation.ko
clean:
	make -C $(KDIR) M=$(shell pwd) clean