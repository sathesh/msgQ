#include <fcntl.h>
#include <stdio.h>

struct sb_msg {
    unsigned short type:3;
    unsigned short len:13;
    unsigned char  data[0];
};

int main()
{
    int fd;
    int len;
    struct sb_msg *msg = malloc(sizeof(struct sb_msg) + 15);

    fd = open("/dev/sbnode", O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return 0;
    }

    len = read(fd, msg, sizeof(struct sb_msg) + 15);
    if (len<0)
    {
        perror("read");
        return 0;
    }

    msg->data[msg->len] = '\0';
    printf ("read: type: %d,  len:%u, data:%s\n", msg->type, msg->len, msg->data);
    return 0;
}
