CC=gcc
CFLAGS = -o
all:
	$(CC) $(CFLAGS) mytunnel mytunnel.c
	$(CC) $(CFLAGS) tunneld tunneld.c
	$(CC) $(CFLAGS) rcv traffic_rcv.c
	$(CC) $(CFLAGS) snd traffic_snd.c
	$(CC) $(CFLAGS) mp myping.c
	$(CC) $(CFLAGS) mpd mypingd.c
clean:
	rm mytunnel 
	rm tunneld
	rm snd
	rm rcv
	rm mp
	rm mpd
