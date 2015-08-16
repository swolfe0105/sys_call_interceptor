# System Call Interceptor Loadable Kernel Module

This LKM is an example of how to find and manipulate the syscall table. The module searches the kernel address space to find the system call table. Once it is found, it replaces the `mkdir()` system call with another function. This new function executes the original `mkdir()` system call, then prints the arguments provided in the system call to the kernel log.

### Usage

first build and load the kernel module.
```bash
sys_call_interceptor$ make
sys_call_interceptor$ sudo insmod sys_call_interceptor.ko
```
Now the module is loaded. Create a new directory.
```bash
sys_call_interceptor$ mkdir test
```
Now check the kernel log
```bash
sys_call_interceptor$ dmesg
...
sys_call_interceptor: starting...
ys_call_interceptor: sys_call_table found at address: XXXXXXXXXXXXXXXX
sys_call_interceptor: intercepted 'mkdir()' with the following parameters:
sys_call_interceptor: filename = test
sys_call_interceptor: mode = 511
```
Unload module
```bash
sys_call_interceptor$ sudo rmmod sys_call_interceptor
```

