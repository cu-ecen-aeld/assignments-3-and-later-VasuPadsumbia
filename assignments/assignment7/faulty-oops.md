# Faulty Kernel Module Oops

## Command Executed

```sh
echo "hello_world" > /dev/faulty

Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Mem abort info:
  ESR = 0x0000000096000045
  EC = 0x25: DABT (current EL), IL = 32 bits
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
  FSC = 0x05: level 1 translation fault
Data abort info:
  ISV = 0, ISS = 0x00000045
  CM = 0, WnR = 1
user pgtable: 4k pages, 39-bit VAs, pgdp=0000000041b50000
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
Internal error: Oops: 0000000096000045 [#1] SMP
Modules linked in: hello(O) scull(O) faulty(O)
CPU: 0 PID: 163 Comm: sh Tainted: G           O       6.1.44 #1
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
pc : faulty_write+0x10/0x20 [faulty]
lr : vfs_write+0xc8/0x390
sp : ffffffc008db3d20
x29: ffffffc008db3d80 x28: ffffff8001b98d40 x27: 0000000000000000
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
x23: 0000000000000012 x22: 0000000000000012 x21: ffffffc008db3dc0
x20: 000000556a79d990 x19: ffffff8001bf8f00 x18: 0000000000000000
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
x5 : 0000000000000001 x4 : ffffffc000780000 x3 : ffffffc008db3dc0
x2 : 0000000000000012 x1 : 0000000000000000 x0 : 0000000000000000
Call trace:
 faulty_write+0x10/0x20 [faulty]
 ksys_write+0x74/0x110
 __arm64_sys_write+0x1c/0x30
 invoke_syscall+0x54/0x130
 el0_svc_common.constprop.0+0x44/0xf0
 do_el0_svc+0x2c/0xc0
 el0_svc+0x2c/0x90
 el0t_64_sync_handler+0xf4/0x120
 el0t_64_sync+0x18c/0x190
Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
---[ end trace 0000000000000000 ]---
```
## Analysis
-The kernel oops occurred due to a NULL pointer dereference inside the faulty_write() function of the faulty kernel module.
-The echo command attempted to write to /dev/faulty, triggering the write() system call.
-This call eventually invoked the faulty_write() handler in the kernel module, which was intentionally written to dereference a NULL pointer.
-The instruction b900003f in the crash dump is a str wzr, [x1] ARM64 instruction, which tries to write to an address stored in register x1. Since x1 was 0x0, this resulted in a crash.

## Educational Purpose
-This kernel module is designed to demonstrate how a faulty driver can crash the entire system.
-It is used to teach how critical memory safety is in kernel space, where even small mistakes like dereferencing a NULL pointer can lead to unrecoverable crashes.

## Lessons Learned
-Never trust user input blindly in kernel space.
-Always validate pointers before dereferencing.
-Use defensive programming practices when writing kernel modules.
