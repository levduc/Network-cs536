#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>

#define MAX_BUF 150

int main()
{
    int fd;
    char * cmdfifo = "cmdfifo";
    char buf[MAX_BUF];
    /* open, read, and display the message from the FIFO */
	while(1){
		fd = open(cmdfifo, O_RDONLY);
		if(fd >= 0)
		{
		    read(fd, buf, MAX_BUF);
		    printf("Received: %s\n", buf);
		    close(fd);
		}
		else
			continue;
	}
    //return 0;
}