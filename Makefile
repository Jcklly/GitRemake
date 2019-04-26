all: WTF WTFserver

WTFserver: WTFserver.c
	gcc -g -o WTFserver WTFserver.c -lpthread

WTF: WTF.c
	gcc -g -o WTF WTF.c -lssl -lcrypto 

clean:
	rm WTF; rm WTFserver
