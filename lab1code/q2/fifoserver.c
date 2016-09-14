#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_BUF 1024

int main()
{
    int fd;
    char * cmdfifo = "cmdfifo";
    char buf[MAX_BUF];
    /* open, read, and display the message from the FIFO */
	while(1){
		fd = open(cmdfifo, O_RDONLY);
		if(fd >= 0)
			read(fd, buf, MAX_BUF);
			printf("Received: %s\n", buf);
			close(fd);
		else
			continue;
	}
    //return 0;
}