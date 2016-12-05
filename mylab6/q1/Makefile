CC=gcc
CFLAGS = -Wall -o
all:
	git pull
	$(CC) $(CFLAGS) olrouter overlayrouter.c
	$(CC) $(CFLAGS) olbuild overlaybuild.c
	$(CC) $(CFLAGS) mp myping.c
	$(CC) $(CFLAGS) mpd mypingd.c
	$(CC) $(CFLAGS) tfs traffic_snd.c
	$(CC) $(CFLAGS) tfr traffic_rcv.c
clean:
	rm olrouter
	rm olbuild
	rm mp
	rm mpd
	rm tfs
	rm tfr
	git commit -am "dcm"
	git push
