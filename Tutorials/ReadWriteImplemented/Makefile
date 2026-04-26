obj-m += char_driver.o

KDIR = /lib/modules/$(shell uname -r)/build
SIGN_FILE = $(KDIR)/scripts/sign-file
KEY = $(HOME)/module-signing/MOK.key
CERT = $(HOME)/module-signing/MOK.crt

all:
	make -C $(KDIR) M=$(shell pwd) modules
	$(SIGN_FILE) sha256 $(KEY) $(CERT) char_driver.ko
clean:
	make -C $(KDIR) M=$(shell pwd) clean
	rm a.out