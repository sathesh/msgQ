
#define SUPERBOX_MSGQ       0x1
#define SUPERBOX_CHARDRIVER 0x2


#define SB "superbox: "

extern u16 max_devs;
extern u16 worker_type;
extern u8  exiting;
int superbox_chardev_init(void);
void superbox_chardev_exit(void);

enum kworker_type {
WORKER_WORKQ=1,
WORKER_DEALYEDQ,
WORKER_TASKLET,
};
