#include <linux/module.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/unistd.h>    /* __NR_execve */
#include <asm/paravirt.h>    /* write_cr0 */
#include <asm/pgtable.h>     /* pte_mkwrite */


#ifndef __NR_syscall_max
#define __NR_syscall_max 547
#endif


unsigned long *syscall_table = NULL;
pte_t *pte;

unsigned long *original_syscall_table[__NR_syscall_max];

asmlinkage int (*real_finit_module)(int fd, const char *param_values, int flags);
asmlinkage new_finit_module(int __user fd, const char __user *param_values, int __user flags) {
  int res;
  int i;

  pr_info("HOOKED finit_module() %d %s %d\n", fd, param_values, flags);

  res = real_finit_module(fd, param_values, flags);

  for (i = 0; i < __NR_syscall_max; i++) {
    if (original_syscall_table[i] != syscall_table[i]) {
      printk("Syscall %d has been hooked!\n", i);
      printk("delivering a roundhouse kick of freedom to this rootkit: %p->%p\n", syscall_table[i], original_syscall_table[i]);

      write_cr0 (read_cr0() & (~ 0x10000));
      pte->pte |= _PAGE_RW;
      syscall_table[i] = original_syscall_table[i];
      write_cr0 (read_cr0() | 0x10000);
      pte->pte &= ~_PAGE_RW;
    }
  }

  return res;
}

/* remove LKM from procfs and sysfs */
void module_hide(void) {
   list_del(&THIS_MODULE->list);             //remove from procfs
   kobject_del(&THIS_MODULE->mkobj.kobj);    //remove from sysfs
   THIS_MODULE->sect_attrs = NULL;
   THIS_MODULE->notes_attrs = NULL;
}


void hijack_finit_module(void) {
   unsigned int level;
   syscall_table = NULL;

   /* lookup dynamic address of sys_call_table using kallsyms */
   syscall_table = (void *)kallsyms_lookup_name("sys_call_table");

   /* get the page table entry (PTE) for the page containing sys_call_table */
   pte = lookup_address((long unsigned int)syscall_table, &level);

   if (syscall_table != NULL) {
      /* enable writing to sys_call_table using CR0 or PTE method, dont need both */
      write_cr0 (read_cr0() & (~ 0x10000));
      pte->pte |= _PAGE_RW;
      real_finit_module = (void *)syscall_table[__NR_finit_module];
      syscall_table[__NR_finit_module] = &new_finit_module;
      pr_info("ROOTKIT real finit_module is at %p\n", real_finit_module);
      pr_info("ROOTKIT hooked finit_module is at %p\n", new_finit_module);
      write_cr0 (read_cr0() | 0x10000);
      pte->pte &= ~_PAGE_RW;
   } else {
      printk(KERN_EMERG "ROOTKIT sys_call_table is NULL\n");
   }
}


void un_hijack_finit_module(void) {
   if (syscall_table != NULL) {
      write_cr0(read_cr0() & (~ 0x10000));
      pte->pte |= _PAGE_RW;
      syscall_table[__NR_finit_module] = real_finit_module;
      write_cr0(read_cr0() | 0x10000);
      pte->pte &= ~_PAGE_RW;
   } else {
      printk(KERN_EMERG "ROOTKIT syscall_table is NULL\n");
   }
}



static int __init my_init(void) {
  int	i;

  syscall_table = (void *)kallsyms_lookup_name("sys_call_table");

  //module_hide();
  hijack_finit_module();

  for (i = 0; i < __NR_syscall_max; i++)
    original_syscall_table[i] = syscall_table[i];

  return 0;
}


static void __exit my_exit(void) {
   un_hijack_finit_module();
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Daniel Roberson");
MODULE_LICENSE("GPL v2");
