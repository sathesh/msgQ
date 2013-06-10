#include <linux/module.h>
#include <linux/kernel.h>

#include "superbox.h"

/* Max no. of char device to be supported 
 * By default it is 2 
 */ 
u16 max_devs = 2;
/* default buffer size of each device */
u32 buf_size = 2048;

int __init superbox_init(void)
{
   pr_notice("Installing superbox"); 

   superbox_chardev_init();

   return 0;
}

void __exit superbox_exit(void)
{
   pr_notice("Uninstalling superbox"); 
   superbox_chardev_exit();
}

module_init(superbox_init);
module_exit(superbox_exit);
module_param(max_devs, ushort, 0644);
module_param(buf_size, uint, 0644);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Satesh");
MODULE_DESCRIPTION("Char device driver - For learning kernel internals and device driver");

