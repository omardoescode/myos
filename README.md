# MyOS

Currently building using this as a guide: https://operating-system-in-1000-lines.vercel.app

## Notes
- No stderr/stdout distinction — all output goes directly to serial via SBI putchar
- No per-process file descriptors — file syscalls use filenames directly
- Entire disk is read into RAM at boot — simple but doesn't scale

## Next Steps
- [ ] Proper memory allocator with freeing (replace bitmap with something better)
- [ ] Interrupt-driven disk I/O (no busy-wait)
- [ ] Full filesystem (ext2 would be a good start)
- [ ] Network communication via virtio-net (UDP/IP first, TCP later)
- [ ] Study xv6 RISC-V — UNIX-like teaching OS with book
- [ ] Study Starina — microkernel OS in Rust
