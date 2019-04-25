#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

	// Gets the command given from the protocol.
	// 1 = create
int getCommand(char* buf) {

	int i = 0;
	while(i < strlen(buf)) {
		if(buf[i] == ':') {
			break;
		}	
		++i;	
	}

	char command[i+1];
	memcpy(command, buf, i);
	command[i] = '\0';

	if(strcmp(command, "create") == 0) {
		return 1;
	}
	
	return 0;
}

	// Gets the <project name> field
char* getProjectName(char* buf) {

	int i, count, bytes;
	i = count = bytes = 0;

	while(i < strlen(buf)) {
		if(count >= 2) {
			break;
		}
		if(buf[i] == ':') {
			++count;
		}
		++i;
	}

	char* name = malloc((strlen(buf) - i + 1)*sizeof(char));
	memcpy(name, buf + i, (strlen(buf) - i));
	name[strlen(buf) - i] = '\0';


	return name;	


}

	// 3.6 -- create
int create(char* projName, int sockfd) {



	// Check if projName already exits.
	DIR *d = opendir(".server");		
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
						fprintf(stderr, "Project: '%s' already exists on server.\n", projName);
						return 0;
					}
				}
			}
			status = readdir(d);
		} while(status != NULL);
		closedir(d);
	}

	// If here, then project does not already exists. Proceed.
	char newDir[strlen(projName) + 21];
	strcpy(newDir, "./.server/");
	strcat(newDir, projName);
	mkdir(newDir, 0700);

	// Initialize .manifest inside new project directory.

	strcat(newDir, "/.Manifest");
	
        int fd = open(newDir, O_CREAT | O_RDWR, 0644);

        if(fd < 0) {
                fprintf(stderr, "Configure never ran. Please run:\n./WTF configure <IP> <PORT>\n");
                exit(1);
        }

	write(fd, "1", 1);

	close(fd);

	
	return 0;
}


int main(int argc, char** argv) {

	if(argc != 2) {
		fprintf(stderr, "Invalid Number of Arguments.\nExpected a single Port #.\nReceived %d argument(s).\nUsage: ./WTFserver <PORT#>\n", argc - 1);
		exit(1);
	}

	int sockfd = -1;
	int newsockfd = -1;
	int clientAddrInfo = -1;

	
		// Convert port string to int
	int port = atoi(argv[1]);
	if( port == 0 ) {
		fprintf(stderr, "Invalid Port.\n");
		exit(1);
	}	

		// Create server socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if( sockfd == -1 ) {
		fprintf(stderr, "Error creating server socket.\n");
		exit(1);
	}

	
		// Server address/port struct
	struct sockaddr_in serverAddr;

		// Initialize server Addr
	bzero((char*)&serverAddr, sizeof(serverAddr));	

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = INADDR_ANY;


		// Check return of binding
	if( bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0 ) {
		fprintf(stderr, "Error while binding server.\n");
		exit(1);
	}

		// Listen for client connections
	listen(sockfd, 25);

	
		// Struct for client
	struct sockaddr_in clientAddr;
	
	clientAddrInfo = sizeof(clientAddr);

	newsockfd = accept(sockfd, (struct sockaddr*)&clientAddr, &clientAddrInfo); 
	if(newsockfd >= 0) {
		printf("Successfully connected to a client.\n");
	}

	int n, i, j;
	i = n = j = 0;


	fd_set set;
	struct timeval timeout;

	FD_ZERO(&set);
	FD_SET(newsockfd, &set);

	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	select(FD_SETSIZE, &set, NULL, NULL, &timeout);

	i = ioctl(newsockfd, FIONREAD, &n);
	
	if(i < 0) {
		fprintf(stderr, "Error with ioctl().\n");
		exit(1);
	}
	
	char buffer[n+1];
	bzero(buffer, n+1);

	if(n > 0) {
		n = read(newsockfd, buffer, n);
	}

//	printf("%s\n", buffer);

	n = write(newsockfd, "Successfully connected to client.", 30);

		// Get the command given from client. (Create, history, rollback, etc...)
		// 1 -- create
	int command = 0;
	command = getCommand(buffer);

	if(command == 1) {
		char* projectName = getProjectName(buffer);
		create(projectName, newsockfd);
		free(projectName);
	}



	close(newsockfd);
	close(sockfd);

	return 0;
}





