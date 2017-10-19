MODULE_NAME  = hello
obj-m       := $(MODULE_NAME).o   

module_ver   = "4.9.35-v7+"
#$(shell uname -r)

all:
	make -C /lib/modules/$(module_ver)/build M=$(shell pwd) modules
clean:
	make -C /lib/modules/$(module_ver)/build M=$(shell pwd) clean
