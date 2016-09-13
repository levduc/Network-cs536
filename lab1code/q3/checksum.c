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
//char to int64
uint64_t charTo64bitNum(unsigned char a[]) {
  uint64_t n = 0;
  memcpy(&n, a, 8);
  return n;
}

uint64_t checksum(unsigned char *ptr)
{
	int file1;
    uint64_t csum = 0; // 64 bit
    file1 = open(ptr, O_RDONLY); //read only
    if (file1 >= 0)
	{
	    unsigned char c;
	    while (read(file1, &c, 1) == 1)
	    {
	    	csum += (int64_t) c;
	    }    
	}
	else
	{
		//fail to open		
	}
	return csum;
}

int myunchecksum(unsigned char *file1, unsigned char *file2)
{
	int f1;
    f1 = open(file1, O_RDONLY); //read only
    uint64_t checkf1 = 0; // 64 bit
    unsigned char checksum1[8]; // 64 bit
    off_t filesize = fsize(file1);
    off_t count = 0;
    if (f1 >= 0 && filesize >= 8)
	{
	    unsigned char c;
	    while (read(f1, &c, 1) == 1)
	    {
	    	if(count != filesize - 7)
	    		checkf1 += (int64_t) c;
	    	if(count == filesize - 7)
	    	{
	    		//checksum1[filesize - count] = c;
	    		checksum1[filesize - count] = c;
	    		//printf("%u\n", (unsigned int) c);
	    	}	
	    	count += 1;
	    }    
	}
	else
	{
		//fail to open	
		printf("file is too small\n");	
	}
	int64_t checkFinal = charTo64bitNum(checksum1);
	int64_t checkFinal1 = checksum(file2);
	if(checkFinal1 == checkFinal)
		return 1;
	return checkFinal1;
}

void writeToFile(unsigned char *file1, unsigned char *file2)
{
	int f1;
	int f2;
	//Open File
	f1 = open(file1, O_RDONLY);
	f2 = open(file2, O_CREAT | O_WRONLY | O_TRUNC);
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
	}    
    //get checksum of file1
    uint64_t checksum1 = checksum(file1);
    unsigned char a[8]; //8bytes
    int64ToChar(a, checksum1);
	for (int i = 7; i >=0; i--)
	{
		//printf("%u \n", (unsigned int) a[i]);
		write(f2, &a[i], 1);
	}
	close(f1);
	close(f2);
}

int main(int argc, char* argv[])
{
    uint64_t checksum1 = checksum(argv[1]);
	printf("%" PRIu64 "\n",checksum1);
	// printf("%jd\n", (intmax_t) fsize(argv[1]));
 	// unsigned char a[sizeof(uint64_t)];
 	// int64ToChar(a, checksum1);
	// printf("%u",(unsigned int) a[0]);
	// char c;
	// while (read(a, &c, 1) == 1) {
	// 	putchar(c);
	// }
	//printf("%" PRIu64 "\n",checksum1);
	writeToFile(argv[1],argv[2]);
    printf("%d",myunchecksum(argv[2],argv[1]));
	//uint64_t dcm = 100;
	//unsigned char a[8];
 	//int64ToChar(a, dcm);
	//printf("%" PRIu64 "\n",charTo64bitNum(a));
    return 0;
}