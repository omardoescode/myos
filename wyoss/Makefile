# building a 32 bit OS. Ignore the
GPPPARAMS = -m32 -Iinclude -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-stack-protector -fno-sized-deallocation -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -mno-avx -Wno-write-strings
ASPARAMS = --32
LDPARAMS = -melf_i386

objects = obj/loader.o \
          obj/gdt.o \
          obj/drivers/driver.o \
          obj/hardwarecommunication/port.o \
          obj/hardwarecommunication/interruptstubs.o \
          obj/hardwarecommunication/interrupts.o \
          obj/drivers/amd_am79c973.o \
          obj/drivers/ata.o \
          obj/hardwarecommunication/pci.o \
          obj/drivers/keyboard.o \
          obj/drivers/mouse.o \
          obj/drivers/vga.o \
					obj/gui/widget.o \
					obj/gui/window.o \
					obj/gui/desktop.o \
					obj/net/ethernetframe.o \
					obj/net/arp.o \
					obj/net/ipv4.o \
					obj/net/icmp.o \
					obj/net/udp.o \
					obj/net/tcp.o \
					obj/filesystem/msdospart.o \
					obj/filesystem/fat.o \
					obj/multitasking.o \
					obj/syscall.o \
					obj/memorymanagement.o \
          obj/kernel.o 

.PHONY: clean

obj/%.o: src/%.cpp
	mkdir -p $(@D)
	g++ $(GPPPARAMS) -o $@ -c $<

obj/%.o: src/%.s
	mkdir -p $(@D)
	as $(ASPARAMS) -o $@ $<

mykernel.bin: linker.ld $(objects)
	ld $(LDPARAMS) -T $< -o $@ $(objects)

install: mykernel.bin
	sudo cp mykernel.bin /boot/mykernel.bin

mykernel.iso: mykernel.bin
	mkdir -p iso/boot/grub
	cp $< iso/boot
	echo 'set timeout=0' >> iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo '' >> iso/boot/grub/grub.cfg
	echo 'menuentry "My Operating System" {' >> iso/boot/grub/grub.cfg
	echo '  multiboot /boot/mykernel.bin' >> iso/boot/grub/grub.cfg
	echo '  boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue --output=$@ iso
	rm -rf iso

hdd.img:
	qemu-img create $@ 512M

run: mykernel.iso hdd.img
	qemu-system-i386 -boot d -cdrom mykernel.iso -hda hdd.img -netdev user,id=n0 -device pcnet,netdev=n0

clean:
	rm -rf obj mykernel.bin mykernel.iso
