CC=gcc
CFLAGS = -Wall -o
all:
	git pull
	$(CC) $(CFLAGS) olrouter overlayrouter.c
	$(CC) $(CFLAGS) olbuild overlaybuild.c
	$(CC) $(CFLAGS) mp myping.c
	$(CC) $(CFLAGS) mpd mypingd.c
clean:
	rm olrouter
	rm olbuild
	rm mp
	rm mpd
	git commit -am "dcm"
	git push
