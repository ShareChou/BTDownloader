Makefile

	#指定编译器
	CC=gcc 
	
	# options for development
	CFLAGS = -Iinclude -Wall -g -DDEBUG
	LDFLAGS=-L./lib -Wl,-rpath=./lib -Wl,-rpath=/usr/local/lib
	
	ttorrent: main.o parse_mefafile.o tracker.o bitfield.o sha1.o message.o peer.o data.o
	policy.o torrent.o bterro.o log.o signal_hander.o
		&(CC) -o $@ $(LDFLAGS) $^ -ldl
		
	clean:
		rm -rf *.o ttorrent

