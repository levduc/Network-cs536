#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>

off_t fsize(const char *filename) {
    struct stat st; 

    if (stat(filename, &st) == 0)
        return st.st_size;

    return -1; 
}

//convert int 64 to char
void int64ToChar(unsigned char a[], uint64_t n) {
  memcpy(a, &n, 8);
}

uint64_t checksum(unsigned char *ptr)
{
	int file1;
    uint64_t csum = 0; // 64 bit
    file1 = open(ptr, O_RDWR); //read only
    if (file1 >= 0)
	{
	    unsigned char c;
	    while (read(file1, &c, 1) == 1)
	    {
	    	csum += (uint64_t) c;
	    }    
	}
	else
	{
		//fail to open
		printf("Fail to open\n");		
		csum = -1;
	}
	close(file1);
	return csum;
}

int mychecksum(unsigned char *file1, unsigned char *file2)
{
	int f1;
	int f2;
	//Open File
	f1 = open(file1, O_RDWR);
	f2 = open(file2, O_RDWR|O_CREAT|O_TRUNC, 0666);
	if (f1 >= 0 && f2 >= 0)
	{
		unsigned char c;
		while (read(f1, &c, 1) == 1) {
	    	write(f2, &c, 1);
		}
	}
	else
	{
		//fail
		printf("Fail to open !");
		return -1;
	}    
    //get checksum of file1
    uint64_t checksum1 = checksum(file1);
    if(checksum1 != -1)
    {
	    unsigned char a[8]; //8bytes
		printf("Checksum is = 0x%" PRIx64 "\n", checksum1);
	    int64ToChar(a, checksum1);
		int i;
		for (i = 7; i >=0; i--)
		{
			write(f2, &a[i], 1);
		}
		close(f1);
		close(f2);
		return 1;
	}
	else{
		close(f1);
		close(f2);
	}
	return -1;
}

int main(int argc, char* argv[])
{
	if(mychecksum(argv[1],argv[2]) != 1)
	{
		printf("Error !");
	}
    return 0;
}