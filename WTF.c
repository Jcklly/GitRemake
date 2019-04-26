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
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/time.h>

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

	int n, i, recLength;
	n = i = recLength = 0;
	
	char sendBuf[11 + strlen(projName)];
	bzero(sendBuf, 11 + strlen(projName));

	char pnBytes[2];
	snprintf(pnBytes, 10, "%d", strlen(projName));

	strcpy(sendBuf, "create:");
	strcat(sendBuf, pnBytes);
	strcat(sendBuf, ":");
	strcat(sendBuf, projName);

	n = write(sockfd, sendBuf, strlen(sendBuf));
	
        fd_set set;
        struct timeval timeout;

        FD_ZERO(&set);
        FD_SET(sockfd, &set);

        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        select(FD_SETSIZE, &set, NULL, NULL, &timeout);

        i = ioctl(sockfd, FIONREAD, &recLength);

        if(i < 0) {
                fprintf(stderr, "Error with ioctl().\n");
                exit(1);
        }

        char recBuf[recLength+1];
        bzero(recBuf, recLength+1);

	if(recLength > 0) {
		n = read(sockfd, recBuf, recLength);
	}
	
	if(recLength == 0) {
		printf("Project name: `%s` already exists on server.\n", projName);
		return;
	}
	
	printf("File received from server: `.Manifest`.\nPlease store it in directory: `%s`\nmv .Manifest %s\n", projName, projName);

	
	int fd = open(".Manifest", O_CREAT | O_RDWR, 0644);
	if (fd == -1) {
		printf("Invalid open: `.Manifest`\n");
		return;
	}

	write(fd, recBuf, strlen(recBuf));


	close(fd);
	close(sockfd);

	free(ipAddress);
	free(portS);

}

void destroy(char* projName) {

	int sockfd = -1;
	int newsockfd = -1;
	int addrInfo = -1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		fprintf(stderr, "Error destroying server socket.\n");
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
		// destroy:<strlen(projName):projName>

	int n = 0;
	
	char sendBuf[11 + strlen(projName)];
	bzero(sendBuf, 11 + strlen(projName));

	char pnBytes[2];
	snprintf(pnBytes, 10, "%d", strlen(projName));

	strcpy(sendBuf, "destroy:");
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

	// 3.8 && 3.9 -- Adds or Removes filename from .Manifest on client side
	// flag represents whether we are 'adding' or 'removing'
	// 1 -- add
	// 2 -- remove
void add_or_remove(int flag, char* projName, char* fileName) {

		// Will get set to 1 if it finds the projName
	int check = 0;

		// Check if project exists on the client side.
	DIR *d = opendir(".");		
	struct dirent *status = NULL;

	if(d != NULL) {
		
		status = readdir(d);

		do {
			if( status->d_type == DT_DIR ) { 
				if( (strcmp(status->d_name, ".") == 0) || (strcmp(status->d_name, "..") == 0) ) {
					;
				} else {
						// Project already exists...
					if(strcmp(status->d_name, projName) == 0) {
						check = 1;
					}
				}
			}
			status = readdir(d);
		} while(status != NULL);
		closedir(d);
	}


		// projName never found in directory
	if(check == 0) {
		fprintf(stderr, "Project: `%s` not found on client. Please create the directory before adding files into it.\n", projName);
		exit(1);
	}


		// 1 -- add
		// else -- remove
	if(flag == 1) {
		
		int existCheck = access(fileName, F_OK);
		
			// File doesn't exists on client.
		if(existCheck < 0) {
			fprintf(stderr, "File does not exists on client.\n");
			exit(1);
		}


		// create buffer that will be added to manifest.
		char toAdd[128];
			
		




	} else {
		
	}
	

	
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
	} else if ( (strcmp(argv[1], "add") == 0) || (strcmp(argv[1], "remove") == 0) ) {
	
		if(argc != 4) {
			fprintf(stderr, "Invalid number of arguments for CONFIGURE.\nExpected 2.\nReceived %d\n", argc-2);
			exit(1);
		} else {
			if(strcmp(argv[1], "add") == 0) {
				add_or_remove(1, argv[2], argv[3]);
			} else {
				add_or_remove(2, argv[2], argv[3]);
			}
		}

	} else {
		;
	}


	return 0;
}




















