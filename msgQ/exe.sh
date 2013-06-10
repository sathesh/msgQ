rmmod superbox
make
insmod superbox.ko
dmesg -c
./write
./read
dmesg
