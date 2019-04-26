all: WTF WTFserver

WTFserver: WTFserver.c
	gcc -g -o WTFserver WTFserver.c -lz -lpthread

WTF: WTF.c
	gcc -g -o WTF WTF.c -lz -lcrypto 

clean:
	rm WTF; rm WTFserver
