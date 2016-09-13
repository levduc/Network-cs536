#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <inttypes.h>

// unsigned char checksum (unsigned char *ptr, size_t sz) {
//     unsigned char chk = 0;
//     while (sz-- != 0)
//         chk -= *ptr++;
//     return chk;
// }

uint64_t checksum(unsigned char *ptr)
{
	int file1;
    uint64_t checksum = 0; // 64 bit
    file1 = open(ptr, O_RDONLY); //read only
    if (file1 >= 0)
	{
	    unsigned char c;
	    while (read(file1, &c, 1) == 1)
	    {
	    	checksum += (int64_t) c;
	    }    
	}
	else
	{
		//fail to open		
	}
	return checksum;
}


void writeToFile(unsigned char *file1, unsigned char *file2)
{
	int f1;
	int f2;
	f1 = open(file1, O_RDONLY);
	f2 = open(file2, O_CREAT | O_WRONLY);
	if (f1 >= 0 && f2 >= 0) //open correctly
	{
		unsigned char c;
		while (read(f1, &c, 1) == 1) {
	    	write(f2, &c, 1);
		}
	}
	else
	{
		//fail
	}
	uint64_t csFile1 = checksum(file1);
	printf("%" PRIu64 "\n",csFile1);
	char cs;
	while (read(csFile1, &cs, 1) == 1) {
      write(f2, &cs, 1);
	}

	close(f1);
	close(f2);
}

void int64ToChar(char* a, uint64_t n) {
  memcpy(a, &n, 8);
}

int main(int argc, char* argv[])
{
    uint64_t checksum1 = checksum(argv[1]);
    char a[sizeof(int64_t)];
    int64ToChar(a, checksum1);
	printf("%c",a[1]);
	// char c;
	// while (read(a, &c, 1) == 1) {
	// 	putchar(c);
	// }
	//printf("%" PRIu64 "\n",checksum1);
	//writeToFile(argv[1],argv[2]);
    return 0;
}