CC=gcc
CFLAGS = -o
all:
	$(CC) $(CFLAGS) stream audiostreamd.c
	$(CC) $(CFLAGS) listen audiolisten.c -lpthread
clean:
	rm stream 
	rm listen