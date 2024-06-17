# Assignment 7 Fault Analysis
## Jack Thomson

This is the trace I get when I run `echo "hello_world" > /dev/faulty` within my buildroot instance.

```
# echo "hello_world" > /dev/faulty
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Mem abort info:
  ESR = 0x96000045
  EC = 0x25: DABT (current EL), IL = 32 bits
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
  FSC = 0x05: level 1 translation fault
Data abort info:
  ISV = 0, ISS = 0x00000045
  CM = 0, WnR = 1
user pgtable: 4k pages, 39-bit VAs, pgdp=00000000426a3000
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
Internal error: Oops: 96000045 [#1] SMP
Modules linked in: faulty(O) hello(O) scull(O) [last unloaded: faulty]
CPU: 0 PID: 163 Comm: sh Tainted: G           O      5.15.18 #1
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
pc : faulty_write+0x14/0x20 [faulty]
lr : vfs_write+0xa8/0x2b0
sp : ffffffc008d0bd80
x29: ffffffc008d0bd80 x28: ffffff80026bb300 x27: 0000000000000000
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
x23: 0000000040000000 x22: 000000000000000c x21: 00000055700d33c0
x20: 00000055700d33c0 x19: ffffff8002644300 x18: 0000000000000000
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
x5 : 0000000000000001 x4 : ffffffc0006fc000 x3 : ffffffc008d0bdf0
x2 : 000000000000000c x1 : 0000000000000000 x0 : 0000000000000000
Call trace:
 faulty_write+0x14/0x20 [faulty]
 ksys_write+0x68/0x100
 __arm64_sys_write+0x20/0x30
 invoke_syscall+0x54/0x130
 el0_svc_common.constprop.0+0x44/0xf0
 do_el0_svc+0x40/0xa0
 el0_svc+0x20/0x60
 el0t_64_sync_handler+0xe8/0xf0
 el0t_64_sync+0x1a0/0x1a4
Code: d2800001 d2800000 d503233f d50323bf (b900003f)
---[ end trace 44fda73cee2e47c5 ]---
```

# What this means

The fault is detailing what is a NULL pointer dereference at `0000000000000000` which aligns to what [we would expect
from the source code.](https://github.com/cu-ecen-aeld/ldd3/blob/master/misc-modules/faulty.c#L52) The derefence is at address 0,
which is not an int pointer, so it throws the proceeding information. It then spits out an error code, which corresponds to the FSC
"Level 1 translation fault" i.e. the memory is not properly mapped. The modules that are present are listed and we can see in the
program counter (pc) that the fault occured in the "faulty" module in `faulty_write`. It gives a list of registeres in memory, and the actual call trace where the error occured. This could be used to debug where the error is occuring. The system then reboots and drops the user back at
a login.