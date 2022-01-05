all: fs/EFI/BOOT/BOOTX64.EFI

fs/EFI/BOOT/BOOTX64.EFI: efi.c common.c file.c graphics.c shell.c gui.c main.c
	mkdir -p fs/EFI/BOOT
	x86_64-w64-mingw32-gcc -Wall -Wextra -e Proto_main -nostdinc -nostdlib \
	-fno-builtin -Wl,--subsystem,10 -o $@ $+
	sudo mount /dev/sdb /mnt
	sudo cp fs/EFI/BOOT/BOOTX64.EFI /mnt/EFI/BOOT/BOOTX64.EFI
	sudo umount /mnt
	reboot
run: fs/EFI/BOOT/BOOTX64.EFI
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -hda fat:fs
clean:
	rm -rf *~ fs

.PHONY: clean
