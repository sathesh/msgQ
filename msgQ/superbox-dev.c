#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sched.h>
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
DEFINE_MUTEX(sbdev_lock);
DECLARE_WAIT_QUEUE_HEAD(sbdev_waitq);

u32 msg_count = 0;

static int sb_open(struct inode *in, struct file *filp)
{
    pr_notice(SB "Opening device %u:%u,  mode: 0x%x",
            imajor(in), iminor(in), filp->f_mode);

    if (unlikely(exiting)) {
        return -EIO;
    }

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

    pr_notice(SB "Read: offset: %u, ", *offset);

    if (! (filp->f_mode & FMODE_READ)){
        pr_err(SB "no read permission");
        ret = -EPERM;
        goto end1;
    }

    if (msg_count == 0) {
        pr_err(SB "no msg available");
        if (filp->f_flags & O_NONBLOCK) {
            ret = -EAGAIN;
            goto end1;
        }
        else {
            wait_event_interruptible(sbdev_waitq, (msg_count != 0 || exiting == 1));
        }
    }

    if (unlikely(exiting)) {
        pr_notice(SB "read: module exiting");
        ret = -EIO;
        goto end1;
    }


    mutex_lock(&sbdev_lock);
    if (msg_count == 0) {
        ret = -EINTR;
        goto end2;
    }

    list_for_each_safe(pos, q, &sb_in_list) {
        msg = list_entry(pos, struct sb_msg, list);
        break;
    }

    if (msg != NULL && cnt >= msg->len+sizeof(u16)) {
        if (copy_to_user(buf, (char*)msg + sizeof(msg->list), msg->len+sizeof(u16))) {
            pr_err(SB "user address fault");
            ret = -EFAULT;
            goto end2;
        }
        list_del(pos);
        ret = msg->len+sizeof(u16);
        *offset +=msg->len+sizeof(u16);
        kfree(msg);
        msg_count--;
    }
    else {
        pr_err(SB "User buf length is short");
        ret = 0;
    }

end2:
    mutex_unlock(&sbdev_lock);
end1:
    pr_notice(SB "read: done");
    return ret;
}


static ssize_t sb_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offset)
{
    struct sb_msg *msg;
    ssize_t ret=0;

    pr_notice(SB "write device");

    if (! (filp->f_mode & FMODE_WRITE)){
        pr_err(SB "no write permission");
        ret = -EPERM;
        goto end1;
    }

    if (cnt < 0)
        return 0;


    msg = kmalloc(sizeof(struct sb_msg)+cnt-sizeof(u16), GFP_KERNEL);
    if (msg == NULL) {
        pr_err(SB "No memory");
        ret = -ENOMEM;
        goto end1;
    }

    mutex_lock(&sbdev_lock);

    if (copy_from_user((char*)msg + sizeof(msg->list), buf, cnt)) {
        kfree(msg);
        pr_err(SB "user address fault");
        ret = -EFAULT;
        goto end2;
    }

    ret = cnt;
    list_add(&msg->list,&sb_in_list);
    msg_count++;

    pr_notice(SB "type =%u, len: %u, data:%s",msg->type, msg->len,msg->data);

end2:
    mutex_unlock(&sbdev_lock);
    wake_up_interruptible(&sbdev_waitq);
end1:
    pr_notice(SB "write: done");
    return ret;
}

int superbox_chardev_init()
{
    u32 result = 0;

    pr_notice(SB "Init ");

    if ((result = misc_register(&sb_dev)) < 0) {
        pr_err(SB "misc_register failed: %u", result);
        goto exit_end;
    }

    pr_notice(SB "Init success. %u devices are added. ", max_devs);
    pr_notice(SB "size list = %u, msb_msg = %u", sizeof(struct list_head), sizeof(struct sb_msg));
exit_end:
    return result;
}

void superbox_chardev_exit()
{
    struct sb_msg *msg = NULL;
    struct list_head *pos, *q;
    u32 result = 0;

    pr_notice(SB "exit ");

    wake_up_interruptible(&sbdev_waitq);

    mutex_lock(&sbdev_lock);

    list_for_each_safe(pos, q, &sb_in_list) {
        msg = list_entry(pos, struct sb_msg, list);
        list_del(pos);
        kfree(msg);
        msg_count--;
    }

    mutex_unlock(&sbdev_lock);

    pr_notice(SB "exit msg_count=%u ", msg_count);
    if ((result = misc_deregister(&sb_dev)) < 0) {
        pr_err(SB "misc_deregister failed: %u", result);
        goto exit_end;
    }

exit_end:
    return;
}
