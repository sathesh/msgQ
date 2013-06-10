#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>


#include "superbox.h"

static int sb_open(struct inode *in, struct file *filp);
static int sb_release(struct inode *in, struct file *filp);
static ssize_t sb_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offset);
static ssize_t sb_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offset);

static struct file_operations fops = {
    .open    = sb_open,
    .release = sb_release,
    .read    = sb_read,
    .write   = sb_write,
};

static struct miscdevice sb_dev = {
    .name = "sbdev",
    .fops = &fops,
    .nodename = "sbnode",
};

struct sb_msg {
    struct list_head list;
    u16 type:3;
    u16 len:13;
    u8  data[0];
};

LIST_HEAD(sb_in_list);
LIST_HEAD(sb_out_list);
DEFINE_MUTEX(msg_lock);
u32 msg_count = 0;

static int sb_open(struct inode *in, struct file *filp)
{
    pr_notice(SB "Opening device %u:%u,  mode: 0x%x",
            imajor(in), iminor(in), filp->f_mode);

    return 0;
}

static int sb_release(struct inode *in, struct file *filp)
{
    pr_notice(SB "Releasing device %u:%u",
            imajor(in), iminor(in));


    return 0;
}


static ssize_t sb_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offset)
{
    struct list_head *pos, *q;
    struct sb_msg *msg = NULL;
    ssize_t ret=0;

    pr_notice(SB "Read: offset: %u, cnt:%u", *offset, cnt);

    if (msg_count == 0) {
        pr_err(SB "no msg available");
        return 0;
    }

    list_for_each_safe(pos, q, &sb_in_list) {
        msg = list_entry(pos, struct sb_msg, list);
        break;
    }

    if (msg != NULL && cnt >= msg->len+sizeof(u16))
    {
        if (copy_to_user(buf, (char*)msg + sizeof(msg->list), msg->len+sizeof(u16)))
        {
            pr_err(SB "user address fault");
            return -EFAULT;
        }
        list_del(pos);
        ret = msg->len+sizeof(u16);
        *offset +=msg->len+sizeof(u16);
        kfree(msg);
        msg_count--;
    }
    pr_notice(SB "read: done");
    return ret;
}


static ssize_t sb_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offset)
{
    struct sb_msg *msg;
    pr_notice(SB "write device");

    if (cnt < 0)
        return 0;


    msg = kmalloc(sizeof(struct sb_msg)+cnt-sizeof(u16), GFP_KERNEL);
    if (msg == NULL)
        return -ENOMEM;

    if (copy_from_user((char*)msg + sizeof(msg->list), buf, cnt))
    {
        kfree(msg);
        return -EFAULT;
    }

    list_add(&msg->list,&sb_in_list);
    msg_count++;

    pr_notice(SB "type =%u, len: %u, data:%s",msg->type, msg->len,msg->data);
    return cnt;
}

int superbox_chardev_init()
{
    u32 result = 0;

    pr_notice(SB "Init ");

    if ((result = misc_register(&sb_dev)) < 0)
    {
        pr_err(SB "misc_register failed: %u", result);
        goto exit_end;
    }

    pr_notice(SB "Init success. %u devices are added. Buf size : %u", max_devs, buf_size);
    pr_notice(SB "size list = %u, msb_msg = %u", sizeof(struct list_head), sizeof(struct sb_msg));
exit_end:
    return result;
}

void superbox_chardev_exit()
{
    u32 result = 0;

    pr_notice(SB "exit ");

    if ((result = misc_deregister(&sb_dev)) < 0)
    {
        pr_err(SB "misc_deregister failed: %u", result);
        goto exit_end;
    }

exit_end:
    return;
}
