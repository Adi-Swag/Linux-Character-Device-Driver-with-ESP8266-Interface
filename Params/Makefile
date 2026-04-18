obj-m += passing_params.o

KDIR = /lib/modules/$(shell uname -r)/build
SIGN_FILE = $(KDIR)/scripts/sign-file
KEY = $(HOME)/module-signing/MOK.key
CERT = $(HOME)/module-signing/MOK.crt
PWD = $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules
	$(SIGN_FILE) sha256 $(KEY) $(CERT) passing_params.ko
clean:
	make -C $(KDIR) M=$(PWD) clean