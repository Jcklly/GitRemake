all: WTF WTFserver

WTFserver: WTFserver.c
	gcc -g -o WTFserver WTFserver.c -lpthread

WTF: WTF.c
	gcc -g -o WTF WTF.c

clean:
	rm WTF; rm WTFserver
