#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>

//get filesize
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

int write8ToFile(unsigned char *file1, unsigned char *file2)
{
	int f1;
	int f2;
	//Open File
	f1 = open(file1, O_RDWR);
	f2 = open(file2, O_RDWR | O_CREAT | O_TRUNC, 0666);
	off_t filesize = fsize(file1);
    off_t count = 1;
	if (f1 >= 0 && f2 >= 0)
	{
		unsigned char c;
		while (read(f1, &c, 1) == 1) {
	    	if(count < filesize - 7)
	    	{
	    		write(f2, &c, 1);
	    	}
	    	count +=1;
		}
	}
	else
	{
		//fail
		return -1;
	}    
	close(f1);
	close(f2);
	return 1;
}

int myunchecksum(unsigned char *file1, unsigned char *file2)
{
	int f1;
    f1 = open(file1, O_RDONLY);
    uint64_t checkf1 = 0; // 64 bit
    unsigned char checksum1[8]; // 64 bit
    off_t filesize = fsize(file1);
    off_t count = 1;
    if (f1 >= 0) //open succeed
	{
	    unsigned char c;
	    while (read(f1, &c, 1) == 1) {
	    	if(count < filesize - 7)
	    	{
	    		checkf1 += (uint64_t) c;
	    	}
	    	if(count >= filesize - 7)
	    	{
	    		checksum1[filesize - count] = c;
	    	}
	    	count += 1;
	    }
	}
	else
	{
		//fail to open	
		printf("Error first file\n");
		return -1;	
	}
	uint64_t check8bytes = 0;
	check8bytes = charTo64bitNum(checksum1);
	if(check8bytes == checkf1) //verifying
	{
	 	printf("Succeed ! File is not corrupted. Start Writing !\n");
	 	if(write8ToFile(file1,file2) == 1)
	 	{
	 		printf("Done !\n");
	 		printf("Checksum = 0x%" PRIx64 "\n", check8bytes);
	 		return 1;
	 	}
	 	else {
	 		printf("not able to write\n");
	 		return -1;
	 	}
	}
	else{
		printf("File is corrupted !\n");
	 	printf("Computed Checksum = 0x%" PRIx64 "\n", checkf1);
	 	printf("Stored   Checksum = 0x%" PRIx64 "\n", check8bytes);
		return -1;
	}
	printf("Error !\n");	
	return -1;
}

int main(int argc, char* argv[])
{
    myunchecksum(argv[1],argv[2]);
    return 0;
}