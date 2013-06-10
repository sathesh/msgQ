
#define SUPERBOX_MSGQ       0x1
#define SUPERBOX_CHARDRIVER 0x2

#define SB "superbox: "

extern u16 max_devs;
extern u32 buf_size;
int superbox_chardev_init(void);
void superbox_chardev_exit(void);
