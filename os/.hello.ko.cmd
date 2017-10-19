cmd_/root/os/hello.ko := ld -r -m elf_x86_64 -T ./scripts/module-common.lds --build-id  -o /root/os/hello.ko /root/os/hello.o /root/os/hello.mod.o ;  true
