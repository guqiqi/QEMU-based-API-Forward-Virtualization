#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h> // open
#include <unistd.h> // close
#include <sys/ioctl.h> // ioclt

#include "../virtio-gl-driver/virtio_gl_common.h"

#define error(fmt, arg...) printf("ERROR: "fmt, ##arg)

#define dev_path "/dev/virtio-gl"

int fd = -1;

void open_device()
{
	fd = open(dev_path, O_RDWR);
	if( fd < 0 )
	{
		error("open device %s faild, %s (%d)\n", dev_path, strerror(errno), errno);
		exit (EXIT_FAILURE);
	}
}

void send_cmd_to_device(int cmd, VirtioGLArg *arg)
{
//	if(__builtin_expect(!!(fd==-1), 0))
//		open_device();

	ioctl(fd, cmd, arg);
}

int main(){
    open_device();
    

}