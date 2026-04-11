ENTRY(boot) /* the entry point of the kernel is the `boot` function */

SECTIONS {
    . = 0x80200000; /* the base address */

    .text :{
        KEEP(*(.text.boot)); /* always placed at the beginning */
        *(.text .text.*);
    }

    /*
     * the order is `.text`, `.rodata`, `.data`, and `.bsa`
     * - text: code of the program
     * - rodata: constant data that is read-only
     * - data: read/write data
     * - bss: read/write data with an initial value of zero
     */

    .rodata : ALIGN(4) {
        *(.rodata .rodata.*);
    }

    .data : ALIGN(4) {
        *(.data .data.*);
    }

    .bss : ALIGN(4) {
        __bss = .;
        *(.bss .bss.* .sbss .sbss.*);
        __bss_end = .;
    }

    . = ALIGN(4);
    . += 128 * 1024;
    __stack_top = .;

    . = ALIGN(4096);
    __free_ram = .;
    . += 64 * 1024 * 1024; /* 64MB */
    __free_ram_end = .;
}
