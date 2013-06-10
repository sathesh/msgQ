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
    struct sb_msg *msg = malloc(sizeof(struct sb_msg) + 10);

    strcpy(msg->data, "hi u there");
    msg->len = strlen(msg->data)+1;
    msg->type = 3;

    fd = open("/dev/sbnode", O_WRONLY|O_NONBLOCK);
    if (fd < 0)
        perror("open");

    len = write(fd, msg, sizeof(struct sb_msg) + msg->len);
    if (len<0)
    {
        perror("read");
        return 0;
    }

    return 0;
}
