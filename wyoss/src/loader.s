# Bootloader

.set MAGIC, 0x1badb002
.set FLAGS, (1<< 0 | 1 << 1)
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
  .long MAGIC
  .long FLAGS
  .long CHECKSUM

.section .text
.extern kernelMain
.extern callConstructors
.global loader

loader:
  mov $kernel_stack, %esp
  call callConstructors
  push %eax
  push %ebx
  call kernelMain

# Add another infinite loop, to the one in kernelMain
_stop:
  cli
  hlt
  jmp _stop


# Recall that the stack pointer goes from top to bottom. If we set it to the beginning of kernel.bin, it will overwrite the bootloader, and the BIOS code (which were added to RAM before kernel.bin)
# We will add some empty space
.section .bss
.space 4*1024*1024; # 4 MiB
kernel_stack:
