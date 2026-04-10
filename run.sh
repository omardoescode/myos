set -xue
QEMU=qemu-system-riscv32

CC=clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf  -fuse-ld=lld -fno-stack-protector -ffreestanding -nostdlib"
# -std=c11	Use C11.
# -O2	Enable optimizations to generate efficient machine code.
# -g3	Generate the maximum amount of debug information.
# -Wall	Enable major warnings.
# -Wextra	Enable additional warnings.
# --target=riscv32-unknown-elf	Compile for 32-bit RISC-V.
# -ffreestanding	Do not use the standard library of the host environment (your development environment).
# -fuse-ld=lld	Use LLVM linker (ld.lld).
# -fno-stack-protector	Disable unnecessary stack protection to avoid unexpected behavior in stack manipulation (see #31).
# -nostdlib	Do not link the standard library.
# -Wl,-Tkernel.ld	Specify the linker script.
# -Wl,-Map=kernel.map	Output a map file (linker allocation result).

# Build the kernel
$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf \
  kernel.c common.c sbi.c

# Start Qemu
$QEMU \
  -machine virt \
  -bios default \
  -nographic \
  -serial mon:stdio \
  --no-reboot \
  -echr 0x11 \
  -kernel kernel.elf
