/* System Call Interceptor
 *
 * AUTHOR: Stephen Wolfe
 *
 * This loadable kernel module is an example of how to find and
 * manipulate the syscall table to modify the behavior of system calls.
 *
 * NOTE: supports x86_64 on Intel Only !
 *       execute this program at your own risk
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/types.h>
#include <asm/paravirt.h>

#define PROG_NAME                "sys_call_interceptor"

/* Manipulating Write Protect bit on Register CR0  */
#define BIT_MASK_CR0_WP          (1ULL << 16) 
#define CR0_WP_DISABLE(x)        (write_cr0(x & ~BIT_MASK_CR0_WP))
#define CR0_WP_RESTORE(x)        (write_cr0(x))

static uint64_t **sys_call_table; 
static uint64_t cr0;              /* Stores content of CR0 Register */

asmlinkage int (*ref_sys_mkdir)(const char *filename, int mode);

asmlinkage int new_sys_mkdir (const char *filename, int mode)
{
	int64_t ret;
	ret = ref_sys_mkdir(filename, mode);

	printk(KERN_INFO "%s: intercepted 'mkdir()' with the following parameters:\n", PROG_NAME);
	printk(KERN_INFO "%s: filename = %s\n", PROG_NAME, filename); 
	printk(KERN_INFO "%s: mode = %d\n", PROG_NAME, mode);

	return ret;
}

/* *HACKY* Searches kernel address space for system call table.
 *
 * This technique works with kernels 2.6.* and above which don't 
 * export the system call table in /proc/kallsyms
 *
 * Credits to niriven for introducing me to this technique in the following write up:
 * https://bbs.archlinux.org/viewtopic.php?id=139406
 */
static uint64_t **find_sys_call_table(void)
{
	uint64_t address = PAGE_OFFSET;
	uint64_t **sct;

	while (address < ULLONG_MAX) {
		sct = (uint64_t **)address;

		if (sct[__NR_close] == (uint64_t *) sys_close) 
			return sct;

		address += sizeof(void *);
	}
	
	return NULL;
}

static int __init sys_call_interceptor_start(void) 
{
	printk(KERN_INFO "%s: starting...\n", PROG_NAME);

	if(!(sys_call_table = find_sys_call_table()))
		return -1;

	printk(KERN_INFO "%s: sys_call_table found at address: %p\n", PROG_NAME, sys_call_table);

	cr0 = read_cr0();

	CR0_WP_DISABLE(cr0);

	ref_sys_mkdir = (void *)sys_call_table[__NR_mkdir];
	sys_call_table[__NR_mkdir] = (uint64_t *)new_sys_mkdir;

	CR0_WP_RESTORE(cr0);

	write_cr0(cr0);

	return 0;
}

static void __exit sys_call_interceptor_end(void) 
{

	printk(KERN_INFO "%s: ending...\n", PROG_NAME);

	if(!sys_call_table) {
		printk(KERN_WARNING "\n%s could not restore sys_call_table." 
                            " The table may be corrupted.", PROG_NAME);
		return;
	}

	CR0_WP_DISABLE(cr0);

	/* Restore sys_call_table and CR0 register */
	sys_call_table[__NR_mkdir] = (uint64_t *)ref_sys_mkdir;
	CR0_WP_RESTORE(cr0);
}

module_init(sys_call_interceptor_start);
module_exit(sys_call_interceptor_end);

MODULE_LICENSE("GPL");
