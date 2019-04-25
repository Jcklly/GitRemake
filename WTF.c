#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 9088


	// Returns either IP Address or port from ./.configure file
	// 1 = get port
	// Anything else = get IP Address
char* getConfig(int flag) {

	int fd = open(".configure", O_RDONLY);
	
	if(fd < 0) {
		fprintf(stderr, "Configure never ran. Please run:\n./WTF configure <IP> <PORT>\n");
		exit(1);
	}


	off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
	size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);

	char config[(int)fileSize + 1];

	int rd = read(fd, config, (int)fileSize+1);
	if(rd < 0) {
		fprintf(stderr, "Error reading from ./.configure in function `getConfig()`\n");
		exit(1);
	}

	config[fileSize] = '\0';
	close(fd);

	int i = 0;
	while(i < (int)fileSize) {
		if(config[i] == '\t') {
			break;
		}
		++i;
	}

	if(i == 0) {
		fprintf(stderr, "Error in function `getCOnfig()`\n");
		exit(1);
	}
	
	if(flag == 1) {

		char* port = malloc((i+1)*sizeof(char));
		memcpy(port, config, i);
		port[i] = '\0';

		return port;

	} else {
		char* addr = malloc(((int)fileSize - (i+1))*sizeof(char)+1);
		memcpy(addr, config+(i+1), ((int)fileSize-(i+1)));
		addr[(int)fileSize-(i+1)] = '\0';

		return addr;
	}
}

void create(char* projName) {

	

	int sockfd = -1;
	int newsockfd = -1;
	int addrInfo = -1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		fprintf(stderr, "Error creating server socket.\n");
		exit(1);
	}


//	struct sockaddr_in clientAddr;
	struct sockaddr_in serverAddr;	

		// Obtain IP Address and Port from .configure file
	char* ipAddress = getConfig(2);
	char* portS = getConfig(1);
	int portNum = atoi(portS);

	bzero((char*)&serverAddr, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(ipAddress);
	serverAddr.sin_port = htons(portNum);


	if( connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0 ) {
		fprintf(stderr, "Error connecting client to server.\n");
		exit(1);
	} else {
		printf("Established connection to server.\n");
	}

		// Send project name to server.
		// create:<strlen(projName):projName>

	int n = 0;
	
	char sendBuf[11 + strlen(projName)];
	bzero(sendBuf, 11 + strlen(projName));

	char pnBytes[2];
	snprintf(pnBytes, 10, "%d", strlen(projName));

	strcpy(sendBuf, "create:");
	strcat(sendBuf, pnBytes);
	strcat(sendBuf, ":");
	strcat(sendBuf, projName);

	
	char totalL[10];
	snprintf(totalL, 10, "%d", strlen(sendBuf));
	
//	printf("%s\n", totalL);

//	n = write(sockfd, totalL, strlen(totalL));
	n = write(sockfd, sendBuf, strlen(sendBuf));
	

	close(sockfd);

	free(ipAddress);
	free(portS);

}


	// Attemps to resolve IP address. Error checking.
void resolveIP() {

	char* addr = getConfig(2);

	struct addrinfo *result;
	int err = 0;
	err = getaddrinfo(addr, NULL, NULL, &result);

	if(err != 0) {
		fprintf(stderr, "Error resolving IP Address: %s\nPlease reconfigure with valid IP Address:\n./WTF configure <IP> <PORT>\n", gai_strerror(err));
		exit(1);
	}


	freeaddrinfo(result);
	free(addr);
}

	// 3.0 - Stores IP Address and Port into ./.configure file.
void configure(char* port, char* addr) {
	

	int rmv = remove(".configure");

	int length = strlen(port) + strlen(addr) + 2;
	char str[length];

	strcpy(str, port);
	strcat(str, "\t");
	strcat(str, addr);
	str[length-1] = '\0';

	int fdr = open(".configure", O_CREAT | O_RDWR, 0644);
			
	if (fdr == -1) {
		printf("Invalid Open: ./.configure\n");
		return;
	}

	write(fdr, str, strlen(str));
	close(fdr);

	
}


int main(int argc, char* argv[]) {

		// Checks if no arguments are given
	if( argc < 2 ) {
		fprintf(stderr, "Invalid number of arguments.\nExpected >= 1.\nReceived %d\n", argc-1);
		exit(1);
	}

		// 3.0 - check if 'CONFIGURE' was called
	if( strcmp(argv[1], "configure" ) == 0) {

		if( argc != 4 ) {
	
			fprintf(stderr, "Invalid number of arguments for CONFIGURE.\nExpected 2.\nReceived %d\n", argc-2);
			exit(1);

		} else {
			configure(argv[3], argv[2]);
			return 0;
		}

	} else if (strcmp(argv[1], "create") == 0) {
 		
		if(argc != 3) {

			fprintf(stderr, "Invalid number of arguments for CREATE.\nExpected 1.\nReceived %d\n", argc-2);
			exit(1);
		} else {
			resolveIP();
			create(argv[2]);
		}
	} else {
		;
	}


/*
	int sockfd = -1;
	int newsockfd = -1;
	int addrInfo = -1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		fprintf(stderr, "Error creating server socket.\n");
		exit(1);
	}


	struct sockaddr_in clientAddr;
	struct sockaddr_in serverAddr;	


	char* sendString = "Hello, this message is from the client!";

	bzero((char*)&serverAddr, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddr.sin_port = htons(PORT);


	if( connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0 ) {
		fprintf(stderr, "Error connecting client to server.\n");
		exit(1);
	}

	char buffer[256];
	bzero(buffer, 256);
	int n = 0;
	n = write(sockfd, sendString, 40);
	n = read(sockfd, buffer, 255);

	printf("%s\n", buffer);
*/

	return 0;
}




















