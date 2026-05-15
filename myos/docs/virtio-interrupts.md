# Lesson: Waking a sleeping process on a virtio-blk interrupt

You asked: *"How do I get an interrupt to wake up the process I just put to sleep?"*

Good question. Don't rush to the answer — there are five subtle layers here, and the order matters. Work through them in sequence.

Scope: RV32I, S-mode kernel, QEMU `virt` machine, MMIO virtio (legacy v1).

---

## 0. Before anything else: read your own code

Open `process.c:185`:

```c
void sleep_current_process() { current_proc->state = PROC_WAITING; }
```

Now your `virtio.c:124-125`:

```c
while (virtq_is_busy(vq))
  sleep_current_process();
```

**Question:** After `sleep_current_process()` returns, where does control go? Look carefully — does the function ever call `yield()` or touch `sstatus`?

Answer yourself before reading on.

...

It doesn't. `state = PROC_WAITING` only *labels* the process — the CPU keeps running the same function on the same stack. So the loop:

1. marks me WAITING,
2. returns immediately,
3. re-checks `virtq_is_busy(vq)` — still busy,
4. marks me WAITING again,
5. ...forever, burning CPU while "asleep".

**Your first fix has nothing to do with interrupts.** What should `sleep_current_process()` do after flipping the state bit, and what's the canonical name of that function in your kernel?

(Hint: you already have `yield()` in `syscall.c:51`. Look at what it does for `PROC_EXITED`.)

---

## 1. Why `while (busy) sleep()` is still wrong even after you fix sleep

Suppose you patch sleep to yield. The loop becomes:

```c
while (virtq_is_busy(vq))
  sleep_current_process();   // now yields
```

Each iteration: check → sleep → (later) woken → check → ...

**Question:** What is `wakeup_processes()` doing to *every* WAITING process (see `process.c:179`)? If a UART RX interrupt fires while you're waiting on the disk, what happens to your process? Is the disk actually done?

The idiomatic shape in kernels is usually expressed as:

```c
while (condition_not_met)
    sleep_on(channel);
```

where `sleep_on` **atomically** (a) registers interest in `channel` and (b) blocks until someone `wakeup`s that channel. The `while` stays — because a wakeup is a *hint*, not a guarantee (spurious wakeups are allowed, and a different waiter may have raced in first).

You don't have channels yet. For now, a single global `wakeup_processes()` + a `while` re-check is acceptable — but know you're accumulating design debt. More on this in §5.

---

## 2. How does virtio actually tell you it's done?

You've been polling `*vq->used_index`. The device bumps that counter after each completed request. But a polling loop is not an interrupt — you want the CPU to *stop* running your thread and be told.

Three registers matter (legacy MMIO, offsets from `VIRTIO_BLK_PADDR`):

| Offset | Name              | Meaning                                             |
|--------|-------------------|-----------------------------------------------------|
| 0x60   | `InterruptStatus` | bit 0: used ring updated. bit 1: config changed.    |
| 0x64   | `InterruptACK`    | write-1-to-clear for the bits you handled.          |
| 0x50   | `QueueNotify`     | (you already use this to kick the device)           |

Spec: https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-1700002 (§4.2.2 MMIO device layout).

**But — there's a gotcha.** Open the spec at §2.7.7 "Used Buffer Notification Suppression":
https://docs.oasis-open.org/virtio/virtio/v1.2/virtio-v1.2.html

Two flag words control whether interrupts fire at all:

- `avail->flags` bit 0 = `VIRTQ_AVAIL_F_NO_INTERRUPT` — *driver says: don't interrupt me on used-ring updates.*
- `used->flags` bit 0 = `VIRTQ_USED_F_NO_NOTIFY` — *device says: don't kick me when you add to avail.*

**Question:** Inspect the `virtio_virtq` struct you allocate in `virtq_init()`. After `alloc_pages`, is the memory guaranteed to be zeroed? If so, what value does `avail.flags` hold right now, and does that mean interrupts are enabled or suppressed?

(Answer: zero means *not* suppressed — good. But confirm `alloc_pages` zeros. If it doesn't, you have a second bug: you may be silently telling the device to never interrupt.)

---

## 3. Wiring virtio-blk into the PLIC

From `virt.dtb`: `virtio_mmio@10001000` has `interrupts = <0x01>`, so IRQ = 1.

You already did UART in `plic.c:12-21`. Mirror the three writes for IRQ 1:

```c
// (this is the pattern — you type it, not me)
PLIC_PRIORITY(VIRTIO_BLK_IRQ) = ?;
PLIC_ENABLE(PLIC_S_CONTEXT) |= (1 << VIRTIO_BLK_IRQ);
// threshold: already set to 0 for UART; leave as-is
```

**Questions:**
1. What priority should you pick? Does it matter relative to UART's priority = 1 when only one IRQ is pending at a time? What happens if both are pending simultaneously and both have priority 1?
2. Where should you add this — `plic_init()` or `virtio_blk_init()`? Argument for each?

(No single right answer. Just justify your choice.)

---

## 4. Dispatching in the trap handler

Your current `trap.c:17-26` assumes every external interrupt is UART:

```c
if (masked == SCAUSE_S_EXTERNAL_INT) {
  while (uart_has_rx()) uart_push(uart_read());
  wakeup_processes();
  plic_complete(irq);
}
```

**Question:** `plic_claim()` returns the IRQ number. You're ignoring it. What should the shape of this block be once two devices can raise external interrupts?

Sketch (high level — you write it):

```
irq = plic_claim()
switch (irq):
    case UART_IRQ:     drain uart rx buffer
    case VIRTIO_BLK_IRQ: handle virtio-blk
    default:           log / panic
wakeup_processes()
plic_complete(irq)
```

### 4a. What goes in the virtio handler body?

Pseudocode — fill in the reads/writes yourself:

1. Read `InterruptStatus` (offset 0x60). Call it `isr`.
2. If `isr & 1` → used ring advanced. That's what you care about.
3. (Optional, for later) if `isr & 2` → config change.
4. Write `isr` back to `InterruptACK` (offset 0x64).
5. Let the outer code call `wakeup_processes()`.

**Question:** Why do you write back the *same value you just read* instead of just writing `1`? Think about: what if between the read and the ACK, bit 1 (config change) got set by the device? What would a blanket `write(1)` do to it?

(Answer: you'd silently lose the config-change notification. Read-modify-ACK is the pattern for "acknowledge exactly what I saw, nothing more." This is also how the interrupt line gets de-asserted — it's level-triggered.)

---

## 5. The wait-queue design smell (name it, don't build it)

Right now `wakeup_processes()` wakes every `PROC_WAITING` process in the table. A UART byte arrives → your disk-waiter wakes up, sees `virtq_is_busy(vq)` is still true, goes back to sleep. Works. Wasteful, but correct — *because* of the `while` loop.

Scale this up: imagine 10 processes waiting on 3 different things. Every interrupt wakes all 10. They all re-check, 9 go back to sleep. This is called a **thundering herd**.

The standard fix: **wait channels / wait queues**. Each waiter associates itself with a "channel" (just a void pointer — often the address of the thing they're waiting for, e.g. `&blk_request_vq`). Wakeup takes a channel argument and wakes only processes sleeping on that channel.

xv6's `sleep(chan, lk)` / `wakeup(chan)` is the canonical minimal implementation:
https://github.com/mit-pdos/xv6-riscv/blob/riscv/kernel/proc.c (search `sleep`, `wakeup`).

**Don't build this today.** Just know the name so you can search for it when you're ready.

---

## 6. The lost-wakeup race (this one WILL bite you)

Look at this sequence with a single CPU. The interrupt from virtio-blk is enabled (SIE in `sstatus`).

```
Thread T                              Interrupt
------------------------------------  -------------------------------------
check virtq_is_busy(vq)  →  true
                                      device finishes, raises IRQ 1
                                      trap: ISR runs, wakeup_processes()
                                        → no process is WAITING yet!
                                      iret
sleep_current_process()               (nothing will ever wake this thread)
yield()   ...forever
```

This is the **lost wakeup** problem. It's canonical. Read: https://pdos.csail.mit.edu/6.828/2023/lec/l-coordination.txt

**Question:** What's the invariant that was violated? Phrase it as: *"between X and Y, no interrupt may fire."*

Two classic solutions — think about which is simpler for your kernel right now:

**(a) Disable interrupts around check+park.**
Clear `SIE` in `sstatus`, check the condition, set state to WAITING, then the scheduler in `yield()` re-enables interrupts as part of context-switching to another process (or to the idle `wfi` loop, which re-enables them before `wfi`). The ISR can't fire between the check and the park, because it's masked.

**(b) ISR sets a flag; sleep checks the flag before parking.**
Something like `vq->pending_wake = 1` in the ISR. `sleep_if_busy(vq)` then does: disable ints → if `pending_wake`, clear it and return without sleeping → else park. Still needs a critical section, just finer-grained.

xv6 uses (a) with a spinlock protecting the condition (the lock is dropped atomically as part of sleep). You don't have spinlocks / SMP yet, so the pure-interrupt-mask version of (a) is simplest.

**Question to yourself:** in RV32I S-mode, which bit of `sstatus` masks supervisor-external interrupts globally? (Hint: `SIE`. Single bit. There's also `sie` the CSR, not to be confused — that one masks specific *sources*.)

---

## Your task, broken down

Don't try to do all of this in one commit. In order:

1. Fix `sleep_current_process()` so it actually yields. Prove it: add a `printf` after the call in `read_write_disk` and watch whether polling burns the CPU.
2. Confirm `avail.flags` is 0 at init (i.e. interrupts not suppressed).
3. Enable IRQ 1 in the PLIC.
4. Extend the trap dispatcher to branch on the `plic_claim()` result.
5. Write the virtio ISR body (read `InterruptStatus`, ACK it, wake).
6. Add the interrupt-mask critical section around the check+sleep in `read_write_disk` to close the lost-wakeup race.
7. Test: `fs_flush()` issues a write. With a breakpoint before `virtq_kick`, does the process actually block (state=WAITING) and then get resumed by the ISR, not by polling?

---

## Reference links

- virtio 1.2 spec (OASIS): https://docs.oasis-open.org/virtio/virtio/v1.2/virtio-v1.2.html
- MMIO device layout §4.2.2: https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-1700002
- Used buffer notification suppression §2.7.7 (in the spec above)
- xv6-riscv virtio_disk.c (canonical reference, read after you've tried): https://github.com/mit-pdos/xv6-riscv/blob/riscv/kernel/virtio_disk.c
- xv6-riscv proc.c (sleep/wakeup on channels): https://github.com/mit-pdos/xv6-riscv/blob/riscv/kernel/proc.c
- Lost wakeup / condition variables (MIT 6.828 notes): https://pdos.csail.mit.edu/6.828/2023/lec/l-coordination.txt
- RISC-V Privileged Spec (sstatus.SIE, sie CSR): https://riscv.org/technical/specifications/ (vol II)

---

## Unresolved issues

- `sleep_current_process()` doesn't yield — confirm & fix first
- verify `alloc_pages` zeros memory (impacts `avail.flags` default)
- pick PLIC priority for VIRTIO_BLK_IRQ vs UART_IRQ
- decide where IRQ-1 enable lives (`plic_init` vs `virtio_blk_init`)
- choose lost-wakeup fix: interrupt-mask vs ISR-sets-flag
- thundering-herd / wait-channel redesign deferred
- config-change (ISR bit 1) handling deferred
