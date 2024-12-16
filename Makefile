#
# M a k e f i l e
#
.PHONY: all clean run qemu requirement
#
EFI_INC_DIR = /usr/include
EFI_LDS_DIR = /usr/lib
EFI_LIB_DIR = /usr/lib
EFI_CFLAGS = -DGNU_EFI_USE_MS_ABI
ARCH = x86_64
QEMU = "qemu-system-x86_64"
#
CC=gcc
#
CFLAGS= $(EFI_CFLAGS) -Wall -Wextra -nostdinc -ffreestanding -ggdb \
        -fpic -fshort-wchar -fno-strict-aliasing -fno-stack-protector   \
        -Ilibc/include \
        -Ilibc/include/uefi \
        -Wsign-compare -I$(EFI_INC_DIR)/efi -I$(EFI_INC_DIR)/efi/x86_64 \
        -include libc/include/uefi/newlib.h

#	    -mno-sse -mno-mmx -std=gnu90 
#
efi_ldflags = \
	-T $(EFI_LDS_DIR)/elf_$(ARCH)_efi.lds \
	-shared \
	-Bsymbolic \
	-nostdlib \
	-znocombreloc \
    -L $(EFI_LIB_DIR) \
	$(EFI_LDS_DIR)/crt0-efi-$(ARCH).o
#
#
systemd_boot         = BOOTX64.EFI
systemd_boot_solib   = boot.so
#
# === Objects:
#
include ./LibcObjs
#
systemd_boot_objects = \
	basic.o \
	ub_util.o \
	ub_test.o \
	undef.o \
	$(LibcObjs)
#
# ===  BOOTX64.EFI を作る.
#
all:	$(systemd_boot)
#
$(systemd_boot): $(systemd_boot_solib)
	objcopy -j .text -j .sdata -j .data -j .dynamic \
	  -j .dynsym -j .rel -j .rela -j .reloc \
	  --target=efi-app-$(ARCH) $< $@

#
# ===  boot.so を作る.
#
$(systemd_boot_solib): $(systemd_boot_objects)
	$(LD) $(efi_ldflags) $(systemd_boot_objects) \
		-o $@ -lefi -lgnuefi $(shell $(CC) -print-libgcc-file-name); \
	nm -D -u $@ | grep ' U ' && exit 1 || :
.DELETE_ON_ERROR: $(systemd_boot_solib)
#
#
# for Ubuntu24.04LTS
#
requirement:
	sudo apt install gnu-efi
	sudo apt install qemu-system-x86
	sudo apt install ovmf
#
#
#
clean:
	rm $(systemd_boot_objects)
	rm -f *.o *.so *.EFI *.so *~ err
	rm -rf fs

qemu:
	-mkdir fs
	-mkdir fs/EFI
	-mkdir fs/EFI/BOOT
	cp BOOTX64.EFI fs/EFI/BOOT/
	qemu-system-x86_64 -bios OVMF.fd -drive file=fat:rw:fs
#
# ===  実行
#
run: qemu
#
