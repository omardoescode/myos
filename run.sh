set -xue
QEMU=qemu-system-riscv32

CC=$(which clang)
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf  -fuse-ld=lld -fno-stack-protector -ffreestanding -nostdlib"
OBJCOPY=$(which llvm-objcopy)

# Build the shell (application)
$CC $CFLAGS -Wl,-Tuser.ld -Wl,-Map=shell.map -o shell.elf shell.c user.c common.c syscall.c
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary shell.elf shell.bin
$OBJCOPY -Ibinary -Oelf32-littleriscv shell.bin shell.bin.o

# Build the kernel
bear -- $CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf \
  kernel.c plic.c common.c sbi.c process.c paging.c utils.c userland.c trap.c virtio.c fs.c uart.c shell.bin.o

# Create a TAR disk
(cd disk && tar cf ../disk.tar --format=ustar *.txt)

# Start Qemu
$QEMU \
  -machine virt \
  -bios default \
  -nographic \
  -serial mon:stdio \
  --no-reboot \
  -echr 0x11 \
  -d unimp,guest_errors,int,cpu_reset -D qemu.log \
  -drive id=drive0,file=disk.tar,format=raw,if=none \
  -device virtio-blk-device,drive=drive0,bus=virtio-mmio-bus.0 \
  -kernel kernel.elf
