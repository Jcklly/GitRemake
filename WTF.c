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
#include <openssl/sha.h>
#include "helper.h"

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
		fprintf(stderr, "Error in function `getConfig()`\n");
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

	struct hostent *server;
	struct sockaddr_in serverAddr;	

		// Obtain IP Address and Port from .configure file
	char* ipAddress = getConfig(2);
	char* portS = getConfig(1);
	int portNum = atoi(portS);
	server = gethostbyname(getConfig(2));
   
	if (server == NULL) {
	  fprintf(stderr,"ERROR, no such host\n");
	  free(portS);
	  free(ipAddress);
	  exit(0);
	}
   
	bzero((char*)&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	//serverAddr.sin_addr.s_addr = inet_addr(ipAddress);
	bcopy((char *)server->h_addr, (char *)&serverAddr.sin_addr.s_addr, server->h_length);
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
		close(sockfd);
		free(ipAddress);
		free(portS);
		return;
	}
	
	//printf("File received from server: `.Manifest`.\nPlease store it in directory: `%s`\nmv .Manifest %s\n", projName, projName);
	///
	char newDir[strlen(projName) + 21];
	strcpy(newDir, "./");
	strcat(newDir, projName);
	mkdir(newDir, 0700);
	///
	strcat(newDir, "/.Manifest");
	int fd = open(newDir, O_CREAT | O_RDWR, 0644);
	if (fd == -1) {
		printf("Invalid open: `.Manifest`\n");
		return;
	}else{
		printf("'%s' folder created with '.Manifest` file initalized inside\n", projName, projName);
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


	struct hostent *server;
	struct sockaddr_in serverAddr;	

		// Obtain IP Address and Port from .configure file
	char* ipAddress = getConfig(2);
	char* portS = getConfig(1);
	int portNum = atoi(portS);
	server = gethostbyname(getConfig(2));
   
	if (server == NULL) {
		free(ipAddress);
		free(portS);
	  	fprintf(stderr,"ERROR, no such host\n");
	  	exit(0);
	}
   
	bzero((char*)&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	//serverAddr.sin_addr.s_addr = inet_addr(ipAddress);
	bcopy((char *)server->h_addr, (char *)&serverAddr.sin_addr.s_addr, server->h_length);
	serverAddr.sin_port = htons(portNum);


	if( connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0 ) {
		fprintf(stderr, "Error connecting client to server.\n");
		exit(1);
	} else {
		printf("Established connection to server.\n");
	}

		// Send project name to server.
		// destroy:<strlen(projName):projName>

	int n, i, recLength;
	n = i = recLength = 0;
	
	char sendBuf[12 + strlen(projName)];
	bzero(sendBuf, 12 + strlen(projName));

	char pnBytes[2];
	snprintf(pnBytes, 10, "%d", strlen(projName));

	strcpy(sendBuf, "destroy:");
	strcat(sendBuf, pnBytes);
	strcat(sendBuf, ":");
	strcat(sendBuf, projName);

	n = write(sockfd, sendBuf, strlen(sendBuf));
	
        fd_set set;
        struct timeval timeout;

        FD_ZERO(&set);
        FD_SET(sockfd, &set);

        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        select(FD_SETSIZE, &set, NULL, NULL, &timeout);

        i = ioctl(sockfd, FIONREAD, &recLength);

        if(i < 0) {
                fprintf(stderr, "Error with ioctl().\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
                exit(1);
        }

        char recBuf[recLength+1];
        bzero(recBuf, recLength+1);

	if(recLength > 0) {
		n = read(sockfd, recBuf, recLength);
	}
	
	if(recLength == 0) {
		printf("Project name: `%s` doesn't exist on server.\n", projName);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}
	

	fprintf(stdout, "Project successfully destroyed!\n");

	close(sockfd);
	free(ipAddress);
	free(portS);

}

	// Rebuilds the .Manifest for a given project using the array of structs of parsed .Manifest info
int rebuildManifest(files* f, char* path, int lines) {


	int i, totalBytes;
	i = totalBytes = 0;

		// Get totalBytes to create buffer that will contain new .Manifest
	while(i < lines) {
	
		totalBytes += (strlen(f[i].file_name) + strlen(f[i].file_hash) + 10);
		++i;
	}

	
	i = 0;
	char buffer[totalBytes];
	char vn[5];
	bzero(buffer, totalBytes);
	
	while(i < lines) {
		if(f[i].version_number == 0) {
			++i;
			continue;
		}
		sprintf(vn, "%d", f[i].version_number);
		if(strcmp(f[i].file_name, ".Manifest") == 0) {
			strcpy(buffer, vn);
			strcat(buffer, "\n");
			bzero(vn, 5);
			
		} else {
			strcat(buffer, vn);
			strcat(buffer, " ");
			strcat(buffer, f[i].file_name);
			strcat(buffer, " ");
			strcat(buffer, f[i].file_hash);
			strcat(buffer, "\n");
			bzero(vn, 5);
		}
		++i;
	}

		// Removes old .Manifest
	int rmv = remove(path);
	if(rmv < 0) {
		fprintf(stderr, "Error removing file: %s\n", path);
		return 0;
	}

	int fd = open(path, O_CREAT | O_RDWR, 0644);
	if(fd < 0) {
		printf("Error opening file: %s\n", path);
		exit(1);
	}
	write(fd, buffer, strlen(buffer));
	close(fd);
	
}

	// Parses fileName, version #, and hash into array of structs. 
void parseManifest(files* f, char* projName, char* fileName) {

	char pathManifest[strlen(projName) + 11];
	strcpy(pathManifest, projName);
	strcat(pathManifest, "/.Manifest");

	int fd = open(pathManifest, O_RDONLY);		
	if(fd < 0) {
		printf("Error opening file: %s\n", fileName);
		exit(1);
	}		

	off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
	size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);

	char buffer[(int)fileSize + 1];
	bzero(buffer, (int)fileSize+1);

	int rfd = read(fd, buffer, (int)fileSize);
	if(rfd < 0) {
		fprintf(stderr, "Error readiing from file: %s", fileName);
		exit(1);
	}
	close(fd);

	buffer[(int)fileSize] = '\0';

	int numElements, line, i;
	i = numElements = line = 0;
	
//	printf("size:\n------- %s\n---------\n", buffer);

	while(buffer[i] != '\n' && i <= (int)fileSize) {
		++i;
	}	


		// Set first file to .Manifest in struct
	strcpy(f[line].file_name, ".Manifest");
	strcpy(f[line].file_hash, "NULL");
	char temp[i+1];
	bzero(temp, i+1);
	memcpy(temp, buffer, i);
	temp[i] = '\0';
	f[line].version_number = atoi(temp);
	++line;
	++i;	

		// Checks if no other file in .Manifest
	if((strlen(buffer)-i) <= 0) {
//		printf("just manifest!\n");
		return;
	}

	char newTemp[strlen(buffer)-i + 1];
	bzero(newTemp, (strlen(buffer)-i+1));
	memcpy(newTemp, buffer+i, strlen(buffer)-i-1);
	newTemp[strlen(buffer)-i] = '\0';


	i = 0;
	while(i < strlen(newTemp)) {
		if(newTemp[i] == ' ') {
			++numElements;
		}
		if(newTemp[i] == '\n') {
			++numElements;
			newTemp[i] = ',';
		}
		++i;
	}
//	printf("%s\n", newTemp);
	
	char* tokenized[numElements+1];
	i = 0;
	tokenized[i] = strtok(newTemp, " ,");
	
	while(tokenized[i] != NULL) {
		tokenized[++i] = strtok(NULL, " ,");
	}


/*	i = 0;
	while(i < numElements) {
		printf("%s\n", tokenized[i]);
		++i;
	}
*/
	i = 0;
	while(i < numElements) {
		if(i%3 == 0) {
			++line;
		}
		if(i+1 > numElements || i+2 > numElements) {
			break;
		}
		f[line-1].version_number = atoi(tokenized[i]);
		strcpy(f[line-1].file_name, tokenized[i+1]);
		strcpy(f[line-1].file_hash, tokenized[i+2]);
		i += 3;
//		printf("%s : %d\n", f[line-1].file_name, line-1);
	}

}


	// Parses the .Update file into uFiles struct
	// return values:
	// -1 - .Update is empty
	// -2 - error reading/opening
	// anything else - good
	// flag == 1 -> .update
	// flag == 2 -> .commit
int parseUpdate(uFiles *f, char* projName, int flag) {

	char pathUpdate[strlen(projName) + 9];
	strcpy(pathUpdate, projName);

	if(flag == 1) {
		strcat(pathUpdate, "/.Update");
	} else {
		strcat(pathUpdate, "/.commit");
	}

	int fd = open(pathUpdate, O_RDONLY);		
	if(fd < 0) {
		printf("Error opening .Update within project: `%s`\n", projName);
		return;
	}		

	off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
	size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);

	if((int)fileSize == 0) {
		fprintf(stdout, "Project is up to date\n");
		close(fd);
		return -1;
	}
	

	char buffer[(int)fileSize + 1];
	bzero(buffer, (int)fileSize+1);

	int rfd = read(fd, buffer, (int)fileSize);
	if(rfd < 0) {
		fprintf(stderr, "Error reading from .Update within project: `%s`", projName);
		close(fd);
		return -2;
	}
	close(fd);
	buffer[(int)fileSize] = '\0';

	int numElements, line, i;
	i = numElements = line = 0;
	
	while(i < strlen(buffer)) {
		if(buffer[i] == ' ') {
			++numElements;
		}
		if(buffer[i] == '\n') {
			++numElements;
			buffer[i] = ',';
		}
		++i;
	}
	
	char* tokenized[numElements];
	i = 0;
	tokenized[i] = strtok(buffer, " ,");
	
	while(tokenized[i] != NULL) {
		tokenized[++i] = strtok(NULL, " ,");
	}

	i = 0;
/*	while(i < numElements) {
		printf("%s\n", tokenized[i]);
		++i;
	}
*/
	i = 0;
	while(i < numElements) {
		if(i%4 == 0) {
			++line;
		}
		if(i+1 > numElements || i+2 > numElements || i+3 > numElements) {
			break;
		}
		strcpy(f[line-1].code, tokenized[i]);
		f[line-1].version_number = atoi(tokenized[i+1]);
		strcpy(f[line-1].file_name, tokenized[i+2]);
		strcpy(f[line-1].file_hash, tokenized[i+3]);
		i += 4;
//		printf("%s : %d : %s : %s\n", f[line-1].code, f[line-1].version_number, f[line-1].file_name, f[line-1].file_hash);

	}

	return 0;

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
		int fd = open(fileName, O_RDONLY);		
		if(fd < 0) {
			printf("Error opening file: %s\n", fileName);
			exit(1);
		}		
	
		off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
		size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
		lseek(fd, cp, SEEK_SET);


			// Generate hash for file.
		char buffer[(int)fileSize];
		bzero(buffer, (int)fileSize);
		int rfd = read(fd, buffer, fileSize);
		if(rfd < 0) {
			fprintf(stderr, "Error readiing from file: %s", fileName);
			exit(1);
		}
		close(fd);
		unsigned char hash[SHA256_DIGEST_LENGTH];
		bzero(hash, SHA256_DIGEST_LENGTH);
		SHA256(buffer, strlen(buffer), hash);
		
			// Converts hash into string.
		char hashString[65];
		int k = 0;
		while(k < SHA256_DIGEST_LENGTH) {
			sprintf(&hashString[k*2], "%02x", hash[k]);
			++k;
		}

	

			// Combine everything to get string to add to .Manifest.
		char toAdd[strlen(hashString) + strlen(fileName) + 10];
		bzero(toAdd, (strlen(hashString) + strlen(fileName) + 10) );
		strcpy(toAdd, "1 ");
		strcat(toAdd, fileName);
		strcat(toAdd, " ");
		strcat(toAdd, hashString);
		strcat(toAdd, "\n");

		char pathManifest[strlen(projName) + 11];
		strcpy(pathManifest, projName);
		strcat(pathManifest, "/.Manifest");

		fd = open(pathManifest, O_APPEND | O_RDWR);
		if(fd < 0) {
			fprintf(stderr, "Error opening file: %s\n", pathManifest);
			exit(1);
		}		

		cp = lseek(fd, (size_t)0, SEEK_CUR);	
		fileSize = lseek(fd, (size_t)0, SEEK_END); 
		lseek(fd, cp, SEEK_SET);

		char manifestBuf[(int)fileSize + 1];

		rfd = read(fd, manifestBuf, (int)fileSize);
		if(rfd < 0) {
			fprintf(stderr, "Error readiing from file: %s\n", pathManifest);
			exit(1);
		}
		
			// Gets number of lines from .Manifest	
		int i = 0;
		int numLines = 0;
		while(i < (int)fileSize) {
			if(manifestBuf[i] == '\n') {
				++numLines;
			}
			++i;
		}	

			// parse .Manifest into array of structs.
		files *f = malloc(numLines*sizeof(*f));
		parseManifest(f, projName, fileName);

		int mCheck = 0;
		i = 0;
		while(i < numLines) {
			if(strcmp(f[i].file_name, fileName) == 0) {
				printf("Warning: File already exists in `.Manifest`.\nHash has been updated.\n");
				bzero(f[i].file_hash, strlen(f[i].file_hash));
				strcpy(f[i].file_hash, hashString);
				mCheck = 1;
				break;
			}
			++i;
		}	
			// Hash value got updated.
			// Need to remake .Manifest based on array of structs.
		if(mCheck == 1) {
			rebuildManifest(f, pathManifest, numLines);
		} else {
			write(fd, toAdd, strlen(toAdd));
			printf("Successfully added file.\n");
		}

		free(f);
		close(fd);

	} else {
		// Remove
		
			// Check if file exists on client.		
		int existCheck = access(fileName, F_OK);
		
			// File doesn't exists on client.
		if(existCheck < 0) {
			fprintf(stderr, "File does not exists on client.\n");
			exit(1);
		}

		
		char pathManifest[strlen(projName) + 11];
		strcpy(pathManifest, projName);
		strcat(pathManifest, "/.Manifest");

		int fd = open(pathManifest, O_RDONLY);
		if(fd < 0) {
			printf("Error opening file: %s\n", fileName);
			exit(1);
		}		

		off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
		size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
		lseek(fd, cp, SEEK_SET);
	
		char manifestBuf[(int)fileSize + 1];

		int rfd = read(fd, manifestBuf, (int)fileSize);
		if(rfd < 0) {
			fprintf(stderr, "Error readiing from file: %s", fileName);
			exit(1);
		}
		
			// Gets number of lines from .Manifest	
		int i = 0;
		int numLines = 0;
		while(i < (int)fileSize) {
			if(manifestBuf[i] == '\n') {
				++numLines;
			}
			++i;
		}	


		
			// Parse .Manifest into array of structs.
		files *f = malloc(numLines*sizeof(*f));
		parseManifest(f, projName, fileName);
		
		i = 0;
		while(i < numLines) {
			
			if(strcmp(fileName, f[i].file_name) == 0) {
					// This gets checked for when rebuilding the .Manifest.
					// If version number is 0, that means the entry should be deleted.
				f[i].version_number = 0;
				break;
			}
		
			++i;
		}


		rebuildManifest(f, pathManifest, numLines);
		printf("Successfully removed file.\n");


		free(f);
		close(fd);
	
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

	// 3.1 -- Checkout
	// Obtain copy of project from server.
void checkout(char* projName) {

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
						fprintf(stderr, "Project already exists on client.\n");
						exit(1);
					}
				}
			}
			status = readdir(d);
		} while(status != NULL);
		closedir(d);
	}

		// If here, then passed all existence check on client side.
	int sockfd = -1;
	int newsockfd = -1;
	int addrInfo = -1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		fprintf(stderr, "Error creating server socket.\n");
		exit(1);
	}


	struct hostent *server;
	struct sockaddr_in serverAddr;	

		// Obtain IP Address and Port from .configure file
	char* ipAddress = getConfig(2);
	char* portS = getConfig(1);
	int portNum = atoi(portS);
	server = gethostbyname(getConfig(2));
   
	if (server == NULL) {
	  fprintf(stderr,"ERROR, no such host\n");
	  free(ipAddress);
	  free(portS);
	  exit(0);
	}
   
	bzero((char*)&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	//serverAddr.sin_addr.s_addr = inet_addr(ipAddress);
	bcopy((char *)server->h_addr, (char *)&serverAddr.sin_addr.s_addr, server->h_length);
	serverAddr.sin_port = htons(portNum);


	if( connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0 ) {
		fprintf(stderr, "Error connecting client to server.\n");
		free(ipAddress);
		free(portS);
		exit(1);
	} else {
		printf("Established connection to server.\n");
	}

		// Send project name to server.
		// checkout:<strlen(projName):projName>

	int n, i, recLength;
	n = i = recLength = 0;
	
	char sendBuf[13 + strlen(projName)];
	bzero(sendBuf, 13 + strlen(projName));
	
	char pnBytes[2];
	snprintf(pnBytes, 10, "%d", strlen(projName));

	strcpy(sendBuf, "checkout:");
	strcat(sendBuf, pnBytes);
	strcat(sendBuf, ":");
	strcat(sendBuf, projName);

	n = write(sockfd, sendBuf, strlen(sendBuf));
	
        fd_set set;
        struct timeval timeout;

        FD_ZERO(&set);
        FD_SET(sockfd, &set);

        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        select(FD_SETSIZE, &set, NULL, NULL, &timeout);

        i = ioctl(sockfd, FIONREAD, &recLength);

        if(i < 0) {
                fprintf(stderr, "Error with ioctl().\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
                exit(1);
        }

        char recBuf[recLength+1];
        bzero(recBuf, recLength+1);

	if(recLength > 0) {
		n = read(sockfd, recBuf, recLength);
	}
	
	if(recLength == 0) {
		printf("Project name: `%s` doesn't exist on server.\n", projName);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}
	printf("Received project: `%s` from server.\n", projName);

	int fd = open("archive.tar.gz", O_CREAT | O_RDWR, 0644);
	if(fd < 0) {
		fprintf(stderr, "Error creating archive.tar.gz in client\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
		
	}
	write(fd, recBuf, recLength);
	close(fd);

		// untar project into current directory
	char command[50];
	strcpy(command, "tar --strip-components=1 -zxf archive.tar.gz");
	int sys = system(command);
	if(sys < 0) {
		fprintf(stderr, "Error untar-ing project received from server.\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}
	
		// Remove old tar file
	int rmv = remove("./archive.tar.gz");
	if(rmv < 0) {
		fprintf(stderr, "Error removing: .server/archive.tar.gz\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}

	close(sockfd);
	free(ipAddress);
	free(portS);
}

	// 3.10 - Gets current version of given project name from the server
void currentVersion(char* projName) {

		// connect to server
	int sockfd = -1;
	int newsockfd = -1;
	int addrInfo = -1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		fprintf(stderr, "Error creating server socket.\n");
		exit(1);
	}


	struct hostent *server;
	struct sockaddr_in serverAddr;	

		// Obtain IP Address and Port from .configure file
	char* ipAddress = getConfig(2);
	char* portS = getConfig(1);
	int portNum = atoi(portS);
	server = gethostbyname(getConfig(2));
   
	if (server == NULL) {
	  fprintf(stderr,"ERROR, no such host\n");
	  free(ipAddress);
	  free(portS);
	  exit(0);
	}
   
	bzero((char*)&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	//serverAddr.sin_addr.s_addr = inet_addr(ipAddress);
	bcopy((char *)server->h_addr, (char *)&serverAddr.sin_addr.s_addr, server->h_length);
	serverAddr.sin_port = htons(portNum);


	if( connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0 ) {
		fprintf(stderr, "Error connecting client to server.\n");
		free(ipAddress);
		free(portS);
		exit(1);
	} else {
		printf("Established connection to server.\n");
	}

		// Send project name to server.
		// currentversion:  :<projName>
	int n, i, recLength;
	n = i = recLength = 0;
	
	char sendBuf[19 + strlen(projName)];
	bzero(sendBuf, 19 + strlen(projName));
	
	char pnBytes[2];
	snprintf(pnBytes, 10, "%d", strlen(projName));

	strcpy(sendBuf, "currentversion:");
	strcat(sendBuf, pnBytes);
	strcat(sendBuf, ":");
	strcat(sendBuf, projName);

	n = write(sockfd, sendBuf, strlen(sendBuf));
	
        fd_set set;
        struct timeval timeout;

        FD_ZERO(&set);
        FD_SET(sockfd, &set);

        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        select(FD_SETSIZE, &set, NULL, NULL, &timeout);

        i = ioctl(sockfd, FIONREAD, &recLength);

        if(i < 0) {
                fprintf(stderr, "Error with ioctl().\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
                exit(1);
        }

        char recBuf[recLength+1];
        bzero(recBuf, recLength+1);

	if(recLength > 0) {
		n = read(sockfd, recBuf, recLength);
	}
	
	if(recLength == 0) {
		printf("Project name: `%s` doesn't exist on server.\n", projName);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}
	
	printf("Received current versions of files from project: `%s` from the server.\nFILE NAME : VERSION NUMBER\n", projName);
	fprintf(stdout, "%s\n", recBuf);


	close(sockfd);
	free(ipAddress);
	free(portS);
}

	// 3.0 - Stores IP Address and Port into ./.configure file.
void configure(char* port, char* addr) {
	
	//printf("%s\n",addr);
	int rmv = remove(".configure");

	int portNum = atoi(port);

	if( (portNum < 8000) || (portNum > 65535) ) {
			// Ensure port is between proper range.
		fprintf(stderr, "Invalid port number. Please make sure the range of the port is between 8000-65535.\n");	
		exit(1);

	}


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

	// 3.2 - Update
void update(char* projName) {

		// connect to server
	int sockfd = -1;
	int newsockfd = -1;
	int addrInfo = -1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		fprintf(stderr, "Error creating server socket.\n");
		exit(1);
	}


	struct hostent *server;
	struct sockaddr_in serverAddr;	

		// Obtain IP Address and Port from .configure file
	char* ipAddress = getConfig(2);
	char* portS = getConfig(1);
	int portNum = atoi(portS);
	server = gethostbyname(getConfig(2));
   
	if (server == NULL) {
	  fprintf(stderr,"ERROR, no such host\n");
	  free(ipAddress);
	  free(portS);
	  exit(0);
	}
   
	bzero((char*)&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	//serverAddr.sin_addr.s_addr = inet_addr(ipAddress);
	bcopy((char *)server->h_addr, (char *)&serverAddr.sin_addr.s_addr, server->h_length);
	serverAddr.sin_port = htons(portNum);


	if( connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0 ) {
		fprintf(stderr, "Error connecting client to server.\n");
		free(ipAddress);
		free(portS);
		exit(1);
	} else {
		printf("Established connection to server.\n");
	}

		// Send project name to server.
		// update:  :<projName>
	int n, i, recLength;
	n = i = recLength = 0;
	
	char sendBuf[11 + strlen(projName)];
	bzero(sendBuf, 11 + strlen(projName));
	
	char pnBytes[2];
	snprintf(pnBytes, 10, "%d", strlen(projName));

	strcpy(sendBuf, "update:");
	strcat(sendBuf, pnBytes);
	strcat(sendBuf, ":");
	strcat(sendBuf, projName);

	n = write(sockfd, sendBuf, strlen(sendBuf));
	
        fd_set set;
        struct timeval timeout;

        FD_ZERO(&set);
        FD_SET(sockfd, &set);

        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        select(FD_SETSIZE, &set, NULL, NULL, &timeout);

        i = ioctl(sockfd, FIONREAD, &recLength);

        if(i < 0) {
                fprintf(stderr, "Error with ioctl().\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
                exit(1);
        }

        char recBuf[recLength+1];
        bzero(recBuf, recLength+1);

	if(recLength > 0) {
		n = read(sockfd, recBuf, recLength);
	}
	
	if(recLength == 0) {
		printf("Project name: `%s` doesn't exist on server.\n", projName);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}

	printf("Received .Manifest from the server. Proceeding to evaluate changes...\n");
	
	int fd = open("archive.tar.gz", O_CREAT | O_RDWR, 0644);
	if(fd < 0) {
		fprintf(stderr, "Error creating archive.tar.gz in client\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
		
	}
	write(fd, recBuf, recLength);
	close(fd);

		// untar project into current directory
	char command[50];
	strcpy(command, "tar --strip-components=2 -zxf archive.tar.gz");
	int sys = system(command);
	if(sys < 0) {
		fprintf(stderr, "Error untar-ing project received from server.\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}
	
		// Remove old tar file
	int rmv = remove("./archive.tar.gz");
	if(rmv < 0) {
		fprintf(stderr, "Error removing: .server/archive.tar.gz\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}


	// Now we have the server's version of the project's .Manifest.
	// Need to compare it with our own project's .Manifest.
	
		// Number of lines in server's .Manifest
	fd = open(".Manifest", O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Error opening .Manifest\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}		

	off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
	size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);

	char server_manifest[(int)fileSize + 1];

	int rfd = read(fd, server_manifest, (int)fileSize);
	if(rfd < 0) {
		fprintf(stderr, "Error readiing from .Manifest\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}
	close(fd);
	server_manifest[(int)fileSize] = '\0';
	
	i = 0;
	int numLines_s = 0;
	while(i < (int)fileSize) {
		if(server_manifest[i] == '\n') {
			++numLines_s;
		}
		++i;
	}	
	

		// Now get the number of lines from the client's .Manifest
	char manifest_c[strlen(projName) + 11];
	strcpy(manifest_c, projName);
	strcat(manifest_c, "/.Manifest");

	fd = open(manifest_c, O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Error opening clients .Manifest in project: %s\n", projName);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}		
	
	fileSize = 0;
	cp = lseek(fd, (size_t)0, SEEK_CUR);	
	fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);

	char client_manifest[(int)fileSize + 1];

	rfd = read(fd, client_manifest, (int)fileSize);
	if(rfd < 0) {
		fprintf(stderr, "Error readiing from clients .Manifest in project: %s\n", projName);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}
	close(fd);
	client_manifest[(int)fileSize] = '\0';	

	i = 0;
	int numLines_c = 0;
	while(i < (int)fileSize) {
		if(client_manifest[i] == '\n') {
			++numLines_c;
		}
		++i;
	}	

		// Parse manifest from server and client into structs to compare.
	files *s = malloc(numLines_s * sizeof(*s));
	files *c = malloc(numLines_c * sizeof(*c));	
//	files *lc = malloc(numLines_c * sizeof(*c));

	parseManifest(s, ".", "Server's .Manifest");
	parseManifest(c, projName, "Client's .Manifest");


		// Check UMAD cases
	int k, UMAD;
	i = k = 1;
	UMAD = 0;

	char updateBuf[strlen(projName) + 9];
	bzero(updateBuf, (strlen(projName) + 9));
	strcpy(updateBuf, projName);
	strcat(updateBuf, "/.Update");
		
	rmv = remove(updateBuf);

	fd = open(updateBuf, O_CREAT | O_RDWR, 0644);
	if(fd < 0) {
		fprintf(stderr, "Error opening file: `%s`.\nThis file is in the client's .Manifest but not in the project.\n", updateBuf);
		close(sockfd);
		free(ipAddress);
		free(portS);
	}
	close(fd);

		// Handles U M D
	while(i < numLines_c) {
		k = 1;
		while(k < numLines_s) {
			
			if(strcmp(c[i].file_name, s[k].file_name) == 0) {
				
					// Get live hash from file on client.
				fd = open(c[i].file_name, O_RDONLY);
				if(fd < 0) {
					fprintf(stderr, "Error opening file: `%s`.\nThis file is in the client's .Manifest but not in the project.\n", c[i].file_name);
					close(sockfd);
					free(ipAddress);
					free(portS);
				}

				cp = lseek(fd, (size_t)0, SEEK_CUR);	
				fileSize = lseek(fd, (size_t)0, SEEK_END); 
				lseek(fd, cp, SEEK_SET);

					// Generate hash for file.
				char buffer[(int)fileSize+1];
				bzero(buffer, (int)fileSize+1);
				int rfd = read(fd, buffer, fileSize);
				if(rfd < 0) {
					fprintf(stderr, "Error reading from file: %s", c[i].file_name);
					close(sockfd);
					free(ipAddress);
					free(portS);
					exit(1);
				}
				close(fd);
				buffer[(int)fileSize] = '\0';

					// Generate live hash
				unsigned char hash[SHA256_DIGEST_LENGTH];
				bzero(hash, SHA256_DIGEST_LENGTH);
				SHA256(buffer, strlen(buffer), hash);
				
					// Converts hash into string.
				char hashString[65];
				bzero(hashString, 65);
				hashString[64] = '\0';
				int p = 0;
				while(p < SHA256_DIGEST_LENGTH) {
					sprintf(&hashString[p*2], "%02x", hash[p]);
					++p;
				}


					// Checks UPLOAD
					// Checks live hash against server's hash and same version .Manifest
				if( (strcmp(s[k].file_hash, hashString) != 0) && (c[0].version_number == s[0].version_number) && (c[i].version_number == s[k].version_number)) {
					fprintf(stdout, "U %s\n", c[i].file_name);
					UMAD = 1;
				} else if( (c[0].version_number == s[0].version_number) && (c[i].version_number != s[k].version_number)) {
						// This case should be impossible. If this gets triggered then someone tampered with one of the manifests
					fprintf(stderr, "Error found. Manifest version is the same but the file version is different. This should be impossible.\n");
					fprintf(stdout, "Files:VN: %s:%d\n%s:%d\n", c[i].file_name, c[i].version_number, s[k].file_name, s[k].version_number);
				}

					// Checks MODIFY
				if(strcmp(hashString, c[i].file_hash) == 0) {
					
					if( (s[0].version_number != c[0].version_number) && (c[i].version_number != s[k].version_number) ) {
						fprintf(stdout, "M %s\n", c[i].file_name);
						UMAD = 1;

						fd = open(updateBuf, O_APPEND | O_RDWR);
						char vn[5];
						bzero(vn, 5);	
						sprintf(vn, "%d", c[i].version_number);

						char toAdd[strlen(c[i].file_name) + strlen(c[i].file_hash) + 12];
						bzero(toAdd, strlen(c[i].file_name) + strlen(c[i].file_hash) + 12);	
						
						strcpy(toAdd, "M ");
						strcat(toAdd, vn);
						strcat(toAdd, " ");
						strcat(toAdd, c[i].file_name);
						strcat(toAdd, " ");
						strcat(toAdd, c[i].file_hash);
						strcat(toAdd, "\n");
				
						write(fd, toAdd, strlen(toAdd));

						close(fd);
					}					

				}
					// Checks another UPLOAD case
				if(strcmp(hashString, c[i].file_hash) != 0) {
					
					if((s[0].version_number != c[0].version_number) && (c[i].version_number == s[k].version_number) ) {
						fprintf(stdout, "U %s\n", c[i].file_name);
						UMAD = 1;
					}
				}

					// Checks for BAD error
				if( (s[0].version_number != c[0].version_number) && (c[i].version_number != s[k].version_number) && (strcmp(hashString, c[i].file_hash) != 0) ) {
					fprintf(stdout, "Conflict found: %s\n", c[i].file_name);
					UMAD = -1;
				}
					
				break;
			}
				
				// Checks UPLOAD
				// File is in client's .Manifest but not in server && version numbers are the same
			if((strcmp(c[i].file_name, s[k].file_name) != 0) && (k == numLines_s-1) && (c[0].version_number == s[0].version_number)) {
				fprintf(stdout, "U %s\n", c[i].file_name);
				UMAD = 1;
			}

				// Checks DELETE
			if( (strcmp(c[i].file_name, s[k].file_name) != 0) && (k == numLines_s-1) && (s[0].version_number != c[0].version_number) ) {
				fprintf(stdout, "D %s\n", c[i].file_name);
				UMAD = 1;

				fd = open(updateBuf, O_APPEND | O_RDWR);
				char vn[5];
				bzero(vn, 5);	
				sprintf(vn, "%d", c[i].version_number);

				char toAdd[strlen(c[i].file_name) + strlen(c[i].file_hash) + 12];
				bzero(toAdd, strlen(c[i].file_name) + strlen(c[i].file_hash) + 12);	
				
				strcpy(toAdd, "D ");
				strcat(toAdd, vn);
				strcat(toAdd, " ");
				strcat(toAdd, c[i].file_name);
				strcat(toAdd, " ");
				strcat(toAdd, c[i].file_hash);
				strcat(toAdd, "\n");
		
				write(fd, toAdd, strlen(toAdd));

				close(fd);
			}
			++k;
		}	
		++i;
	}

		// Handles A
	i = k = 1;
	while(k < numLines_s) {
		i = 1;
		while(i < numLines_c) {
		
//			printf("%s : %s\n", s[k].file_name, c[i].file_name);


			if(strcmp(s[k].file_name, c[i].file_name) == 0) {
				break;
			}
			
			if( (strcmp(s[k].file_name, c[i].file_name) != 0) && (i == numLines_c-1) && (s[0].version_number != c[0].version_number) ) {
				fprintf(stdout, "A %s\n", s[k].file_name);
				UMAD = 1;
	
				fd = open(updateBuf, O_APPEND | O_RDWR);
				char vn[5];
				bzero(vn, 5);	
				sprintf(vn, "%d", s[k].version_number);

				char toAdd[strlen(s[k].file_name) + strlen(s[k].file_hash) + 12];
				bzero(toAdd, strlen(s[k].file_name) + strlen(s[k].file_hash) + 12);	
				
				strcpy(toAdd, "A ");
				strcat(toAdd, vn);
				strcat(toAdd, " ");
				strcat(toAdd, s[k].file_name);
				strcat(toAdd, " ");
				strcat(toAdd, s[k].file_hash);
				strcat(toAdd, "\n");
		
				write(fd, toAdd, strlen(toAdd));

				close(fd);
			}

			++i;
		}
		++k;
	}


	if(UMAD == -1) {
		printf("Conflicts were found while updating. Please refer to the files preceding with `Conflict: <file>` and resolve the issues.\n");

			// Remove .Update if found		
		rmv = remove(updateBuf);

		free(s);
		free(c);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}
	
	if(UMAD == 0) {
		fprintf(stdout, "Up to date\n");
	} 

		// remove .Manifest
	rmv = remove(".Manifest");
	
	free(s);
	free(c);
	close(sockfd);
	free(ipAddress);
	free(portS);
}

	// 3.3 - upgrade
void upgrade(char* projName) {

		// Test is .Update exists. If not, tell client to run ./WTF update
	char updateFile[strlen(projName) + 9];
	bzero(updateFile, (strlen(projName) + 9));
	strcpy(updateFile, projName);
	strcat(updateFile, "/.Update");

	if(access(updateFile, F_OK) < 0) {
		fprintf(stderr, "Error, .Update not found in project: `%s`\nPlease run `./WTF update %s` before upgrading.\n", projName, projName);
		exit(1);
	}
		
		// Connect to server
	int sockfd = -1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		fprintf(stderr, "Error creating server socket.\n");
		exit(1);
	}


	struct hostent *server;
	struct sockaddr_in serverAddr;	

		// Obtain IP Address and Port from .configure file
	char* ipAddress = getConfig(2);
	char* portS = getConfig(1);
	int portNum = atoi(portS);
	server = gethostbyname(getConfig(2));
   
	if (server == NULL) {
	  fprintf(stderr,"ERROR, no such host\n");
	  free(ipAddress);
	  free(portS);
	  exit(0);
	}
   
	bzero((char*)&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	//serverAddr.sin_addr.s_addr = inet_addr(ipAddress);
	bcopy((char *)server->h_addr, (char *)&serverAddr.sin_addr.s_addr, server->h_length);
	serverAddr.sin_port = htons(portNum);


	if( connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0 ) {
		fprintf(stderr, "Error connecting client to server.\n");
		free(ipAddress);
		free(portS);
		exit(1);
	} else {
		printf("Established connection to server.\n");
	}

		// Send project name to server.
		// update:  :<projName>
	int n, i, recLength;
	n = i = recLength = 0;
	
	char sendBuf[12 + strlen(projName)];
	bzero(sendBuf, 12 + strlen(projName));
	
	char pnBytes[2];
	snprintf(pnBytes, 10, "%d", strlen(projName));

	strcpy(sendBuf, "upgrade:");
	strcat(sendBuf, pnBytes);
	strcat(sendBuf, ":");
	strcat(sendBuf, projName);

	n = write(sockfd, sendBuf, strlen(sendBuf));

		// Open .Update and get number of lines
	int fd = open(updateFile, O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Error opening .Update in project: `%s`\n", projName);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}
	
	off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
	size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);
	
	char updateBuf[(int)fileSize + 1];
	bzero(updateBuf, (int)fileSize + 1);

	int rfd = read(fd, updateBuf, (int)fileSize);
	if(rfd < 0) {
		fprintf(stderr, "Error readiing from .Update\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		close(fd);
		exit(1);
	}
	close(fd);
	updateBuf[(int)fileSize] = '\0';
	
	i = 0;
	int numLines = 0;
	while(i < (int)fileSize) {
		if(updateBuf[i] == '\n') {
			++numLines;
		}
		++i;
	}	

	if(numLines == 0) {
		fprintf(stdout, "%s is up to date!\n", projName);
		int rmvv = remove(updateFile);
		if(rmvv < 0) {
			fprintf(stderr, "Error removing .Update file from project: `%s`\n", projName);
			close(sockfd);
			free(ipAddress);
			free(portS);
			exit(1);
		}
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}

		// Struct for holding parsed .Update file.
	uFiles *f = malloc(numLines*sizeof(*f));
	int rtn = parseUpdate(f, projName, 1);
	if(rtn == -2) {
		fprintf(stderr, "Error opening .Update file.\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}

		// Path for .Manifest on client side
	char manifestPath[strlen(projName) + 11];
	bzero(manifestPath, (strlen(projName) + 11));

	strcpy(manifestPath, projName);
	strcat(manifestPath, "/.Manifest");
	fd = open(manifestPath, O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Error opening .Manifest in project: `%s`\n", projName);
		close(sockfd);
		free(ipAddress);
		free(portS);
		free(f);
		exit(1);
	}
	
	cp = lseek(fd, (size_t)0, SEEK_CUR);	
	fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);
	
	char manifestBuf[(int)fileSize + 1];
	bzero(manifestBuf, (int)fileSize + 1);

	rfd = read(fd, manifestBuf, (int)fileSize);
	if(rfd < 0) {
		fprintf(stderr, "Error readiing from .Manifest\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		close(fd);	
		free(f);
		exit(1);
	}
	close(fd);
	manifestBuf[(int)fileSize] = '\0';
	
	i = 0;
	int numLinesM = 0;
	while(i < (int)fileSize) {
		if(manifestBuf[i] == '\n') {
			++numLinesM;
		}
		++i;
	}	
	

		// Struct for .Manifest
	files *m = malloc(numLinesM*sizeof(*m));
	parseManifest(m, projName, ".Manifest");


		// Look through manifest and .Update, if D then delete from .Manifest.
	int k = 0;
	i = 0;
	while(i < numLines) {
		k = 0;
		while(k < numLinesM) {
			if((strcmp(m[k].file_name, f[i].file_name) == 0) && (strcmp(f[i].code, "D") == 0)) {
				m[k].version_number = 0;
			}
			++k;
		}
		++i;
	}		

			// Rebuild the manifest.
		rebuildManifest(m, manifestPath, numLinesM);

	
		// At this point, all DELETE's are finished.
		// Now, send code for files we want from the server to retrieve them.
	int size = 0;
	i = 0;
		// Get size needed for buffer to send to server
	while(i < numLines) {
		if((strcmp(f[i].code, "A") == 0) || (strcmp(f[i].code, "M") == 0)) {
			size += (strlen(f[i].file_name) + 2);
		}
		++i;
	}

	char sendU[size + strlen(projName) + 12];
	bzero(sendU, (size + strlen(projName) + 12));
	sendU[0] = '\0';
	i = 0;
	while(i < numLines) {
		if((strcmp(f[i].code, "A") == 0) || (strcmp(f[i].code, "M") == 0)) {
			strcat(sendU, f[i].file_name);
			strcat(sendU, " ");
		}
		++i;
	}
	strcat(sendU, projName);
	strcat(sendU, "/.Manifest");
	strcat(sendU, " ");

        fd_set set;
        struct timeval timeout;

	recLength = 0;
        FD_ZERO(&set);
        FD_SET(sockfd, &set);

        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

	select(FD_SETSIZE, &set, NULL, NULL, &timeout);

		// Send .Update stuff		
	write(sockfd, sendU, strlen(sendU));

	recLength = 0;
        FD_ZERO(&set);
        FD_SET(sockfd, &set);

        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

	select(FD_SETSIZE, &set, NULL, NULL, &timeout);

	i = ioctl(sockfd, FIONREAD, &recLength);
        if(i < 0) {
                fprintf(stderr, "Error with ioctl().\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		free(f);
		free(m);
                exit(1);
        }

	char recBuf[recLength + 1];
	bzero(recBuf, recLength + 1);

	if(recLength > 0) {
		n = read(sockfd, recBuf, recLength);
	}

	if(recLength == 0) {
		printf("Project name: `%s` doesn't exist on server.\n", projName);
		close(sockfd);
		free(ipAddress);
		free(portS);
		free(f);
		free(m);
		exit(1);
	}

	printf("Received compressed files from server...proceeding to decompress...\n");

		// Contains tar of files
	fd = open("archive1.tar.gz", O_CREAT | O_RDWR, 0644);
	if(fd < 0) {
		fprintf(stderr, "Error creating archive1.tar.gz in client\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		free(f);
		free(m);
		exit(1);
		
	}
	write(fd, recBuf, recLength);
	close(fd);

		// untar project into current directory
	char command[51];
	bzero(command, 51);
	strcpy(command, "tar --strip-components=1 -zxf archive1.tar.gz");
	int sys = system(command);
	if(sys < 0) {
		fprintf(stderr, "Error untar-ing project received from server.\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		free(f);
		free(m);
		exit(1);
	}
	
		// Remove old tar file
	int rmv = remove("./archive1.tar.gz");
	if(rmv < 0) {
		fprintf(stderr, "Error removing: .server/archive.tar.gz\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		free(f);
		free(m);
		exit(1);
	}

		// Remove .Update file from project
	rmv = remove(updateFile);
	if(rmv < 0) {
		fprintf(stderr, "Error removing .Update file from project: `%s`\n", projName);
		close(sockfd);
		free(ipAddress);
		free(portS);
		free(f);
		free(m);
		exit(1);
	}


	close(sockfd);
	free(ipAddress);
	free(portS);
	free(f);
	free(m);
}


	// 3.4 - Commit
void commit(char* projName) {
	
		// Check if .Update file is there, and if it is make sure it is empty.
	char updatePath[strlen(projName) + 8];
	bzero(updatePath, 8);

	strcpy(updatePath, projName);
	strcat(updatePath, "/.Update");

	int acc = -1;
	acc = access(updatePath, F_OK);
	if(acc >= 0) {
		// Check if file is 0 bytes or not
		int fd = open(updatePath, O_RDONLY);
		off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
		size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
		lseek(fd, cp, SEEK_SET);
		close(fd);

			// .Update file found and not empty...
		if((int)fileSize > 0) {
			fprintf(stderr, ".Update file was found in the project and it is not empty. Please do `./WTF upgrade %s` before proceeding with a commit/push.\n", projName);
			exit(1);
		}
	}



		// Connect to server and retrieve .Manifest	
	int sockfd = -1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		fprintf(stderr, "Error creating server socket.\n");
		exit(1);
	}


	struct hostent *server;
	struct sockaddr_in serverAddr;	

		// Obtain IP Address and Port from .configure file
	char* ipAddress = getConfig(2);
	char* portS = getConfig(1);
	int portNum = atoi(portS);
	server = gethostbyname(getConfig(2));
   
	if (server == NULL) {
	  fprintf(stderr,"ERROR, no such host\n");
	  free(ipAddress);
	  free(portS);
	  exit(0);
	}
   
	bzero((char*)&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	//serverAddr.sin_addr.s_addr = inet_addr(ipAddress);
	bcopy((char *)server->h_addr, (char *)&serverAddr.sin_addr.s_addr, server->h_length);
	serverAddr.sin_port = htons(portNum);


	if( connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0 ) {
		fprintf(stderr, "Error connecting client to server.\n");
		free(ipAddress);
		free(portS);
		exit(1);
	} else {
		printf("Established connection to server.\n");
	}

		// Send project name to server.
		// update:  :<projName>
	int n, k, i, recLength;
	n = i = k = recLength = 0;
	
	char sendBuf[11 + strlen(projName)];
	bzero(sendBuf, 11 + strlen(projName));
	
	char pnBytes[2];
	snprintf(pnBytes, 10, "%d", strlen(projName));

	strcpy(sendBuf, "commit:");
	strcat(sendBuf, pnBytes);
	strcat(sendBuf, ":");
	strcat(sendBuf, projName);

	n = write(sockfd, sendBuf, strlen(sendBuf));
	
        fd_set set;
        struct timeval timeout;

        FD_ZERO(&set);
        FD_SET(sockfd, &set);

        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        select(FD_SETSIZE, &set, NULL, NULL, &timeout);

        i = ioctl(sockfd, FIONREAD, &recLength);

        if(i < 0) {
                fprintf(stderr, "Error with ioctl().\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
                exit(1);
        }

        char recBuf[recLength+1];
        bzero(recBuf, recLength+1);

	if(recLength > 0) {
		n = read(sockfd, recBuf, recLength);
	}
	
	if(recLength == 0) {
		printf("Project name: `%s` doesn't exist on server.\n", projName);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}

	printf("Received .Manifest from the server. Proceeding to evaluate changes...\n");
	
	int fd = open("archive.tar.gz", O_CREAT | O_RDWR, 0644);
	if(fd < 0) {
		fprintf(stderr, "Error creating archive.tar.gz in client\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
		
	}
	write(fd, recBuf, recLength);
	close(fd);

		// untar project into current directory
	char command[50];
	strcpy(command, "tar --strip-components=2 -zxf archive.tar.gz");
	int sys = system(command);
	if(sys < 0) {
		fprintf(stderr, "Error untar-ing project received from server.\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}
	
		// Remove old tar file
	int rmv = remove("./archive.tar.gz");
	if(rmv < 0) {
		fprintf(stderr, "Error removing: .server/archive.tar.gz\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}


		// Have the server's .Manifest. Need to evalute changes and compare to our own.
	
		// Number of lines in server's .Manifest
	fd = open(".Manifest", O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Error opening .Manifest\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}		

	off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
	size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);

	char server_manifest[(int)fileSize + 1];

	int rfd = read(fd, server_manifest, (int)fileSize);
	if(rfd < 0) {
		fprintf(stderr, "Error readiing from .Manifest\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}
	close(fd);
	server_manifest[(int)fileSize] = '\0';
	
	i = 0;
	int numLines_s = 0;
	while(i < (int)fileSize) {
		if(server_manifest[i] == '\n') {
			++numLines_s;
		}
		++i;
	}	

		// Now get the number of lines from the client's .Manifest
	char manifest_c[strlen(projName) + 11];
	strcpy(manifest_c, projName);
	strcat(manifest_c, "/.Manifest");

	fd = open(manifest_c, O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Error opening clients .Manifest in project: %s\n", projName);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}		
	
	fileSize = 0;
	cp = lseek(fd, (size_t)0, SEEK_CUR);	
	fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);

	char client_manifest[(int)fileSize + 1];

	rfd = read(fd, client_manifest, (int)fileSize);
	if(rfd < 0) {
		fprintf(stderr, "Error readiing from clients .Manifest in project: %s\n", projName);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}
	close(fd);
	client_manifest[(int)fileSize] = '\0';	

	i = 0;
	int numLines_c = 0;
	while(i < (int)fileSize) {
		if(client_manifest[i] == '\n') {
			++numLines_c;
		}
		++i;
	}	


		// Parse manifest from server and client into structs to compare.
	files *s = malloc(numLines_s * sizeof(*s));
	files *c = malloc(numLines_c * sizeof(*c));
	files *lc = malloc(numLines_c * sizeof(*c));

	parseManifest(s, ".", "Server's .Manifest");
	parseManifest(c, projName, "Client's .Manifest");


		// Check and make sure manifest versions match...
	if(s[0].version_number != c[0].version_number) {
//		printf("%d : %d\n", s[0].version_number, c[0].version_number);

//		printf("%d : %s\n", c[0].version_number, c[0].file_name);

		fprintf(stdout, "client and server manifest versions do not match.\nPlease update your local project first.(update + upgrade)\n");
		close(sockfd);
		free(ipAddress);	
		free(portS);
		free(s);
		free(lc);
		free(c);
		exit(1);
	}

	
		// recompute hash codes for all files inside client's .Manifest
	i = 0;
	while(i < numLines_c) {
		
		fd = open(c[i].file_name, O_RDONLY);
		if(fd < 0) {
			fprintf(stderr, "Error opening file: `%s`.\nThis file is in the client's .Manifest but not in the project.\n");
			close(sockfd);
			free(ipAddress);
			free(portS);
			free(s);
			free(c);
			free(lc);
			exit(1);
		}

		cp = lseek(fd, (size_t)0, SEEK_CUR);	
		fileSize = lseek(fd, (size_t)0, SEEK_END); 
		lseek(fd, cp, SEEK_SET);

			// Generate hash for file.
		char buffer[(int)fileSize+1];
		bzero(buffer, (int)fileSize+1);
		int rfd = read(fd, buffer, fileSize);
		if(rfd < 0) {
			fprintf(stderr, "Error readiing from file: %s", c[i].file_name);
			close(sockfd);
			free(ipAddress);
			free(portS);
			free(s);
			free(c);
			free(lc);
			exit(1);
		}
		close(fd);
		buffer[(int)fileSize] = '\0';

			// Generate live hash
		unsigned char hash[SHA256_DIGEST_LENGTH];
		bzero(hash, SHA256_DIGEST_LENGTH);
		SHA256(buffer, strlen(buffer), hash);
		
			// Converts hash into string.
		char hashString[65];
		bzero(hashString, 65);
		hashString[64] = '\0';
		int p = 0;
		while(p < SHA256_DIGEST_LENGTH) {
			sprintf(&hashString[p*2], "%02x", hash[p]);
			++p;
		}

		lc[i].version_number = c[i].version_number;
		strcpy(lc[i].file_name, c[i].file_name);
		strcpy(lc[i].file_hash, hashString);

		++i;
	}


		// create .commit file with random number appended to it.
	int r = 0;
	srand(time(NULL));
	r = rand() % 5000;
	char appendInt[5];
	bzero(appendInt, 5);

	sprintf(appendInt, "%d", r);

	char commitBuf[strlen(projName) + 15];
	bzero(commitBuf, 15);

	strcpy(commitBuf, projName);
	strcat(commitBuf, "/.commit-");
	strcat(commitBuf, appendInt);

	char commitPath[strlen(projName) + 10];
	strcpy(commitPath, projName);
	strcat(commitPath, "/.commit");
	
	rmv = remove(commitPath);
	
	fd = open(commitPath, O_CREAT | O_RDWR | O_APPEND, 0644);
	close(fd);

		// Now we have the live hashes of all files in clients .Manifest, continue checking...


		// Checks if newly computed hashcode is different than clients
	i = k = 0;
	int check = 0;
	while(i < numLines_c) {
		k = 0;
		while(k < numLines_s) {	

				// Makes sure it doesn't mess with the .Manifest file and version
			if((strcmp(c[i].file_name, ".Manifest") == 0)) {
				break;
			}

				// in both clients and servers .Manifest
			if(strcmp(c[i].file_name, s[k].file_name) == 0) {

					// Checks if file is updated version	
				if(strcmp(lc[i].file_hash, c[i].file_hash) != 0) {

					lc[i].version_number += 1;
					
						// Checks for error
					if(lc[i].version_number <= s[k].version_number) {
						fprintf(stdout, "Error, local .Manifest is not up-to-date with the server. Please synch your project with the server before attempting to commit/push.\n");
						check = -1;
						i = numLines_c;
						k = numLines_s;
						break;
					}


	
					fd = open(commitPath, O_APPEND | O_RDWR);
					char vn[5];
					bzero(vn, 5);	
					sprintf(vn, "%d", lc[i].version_number);

					char toAdd[strlen(lc[i].file_name) + strlen(lc[i].file_hash) + 12];
					bzero(toAdd, strlen(lc[i].file_name) + strlen(lc[i].file_hash) + 12);	
					
					strcpy(toAdd, "U ");
					strcat(toAdd, vn);
					strcat(toAdd, " ");
					strcat(toAdd, lc[i].file_name);
					strcat(toAdd, " ");
					strcat(toAdd, lc[i].file_hash);
					strcat(toAdd, "\n");
			
					write(fd, toAdd, strlen(toAdd));

					printf("%s\n", toAdd);
		
					close(fd);
					check = 1;
				}
				break;


			}	
				
				// File in client .Manifest but not in servers.
			if( (strcmp(c[i].file_name, s[k].file_name) != 0) && (k == numLines_s-1) ) {


//				lc[i].version_number += 1;
		
				fd = open(commitPath, O_APPEND | O_RDWR);
				char vn[5];
				bzero(vn, 5);	
				sprintf(vn, "%d", lc[i].version_number);

				char toAdd[strlen(lc[i].file_name) + strlen(lc[i].file_hash) + 12];
				bzero(toAdd, strlen(lc[i].file_name) + strlen(lc[i].file_hash) + 12);	
				
				strcpy(toAdd, "A ");
				strcat(toAdd, vn);
				strcat(toAdd, " ");
				strcat(toAdd, lc[i].file_name);
				strcat(toAdd, " ");
				strcat(toAdd, lc[i].file_hash);
				strcat(toAdd, "\n");
		
				write(fd, toAdd, strlen(toAdd));

				printf("%s\n", toAdd);
		
				close(fd);
				check = 1;



			}




			++k;
		}

		++i;
	}


		// Checks if it is in the servers and not the clients (REMOVE)
	i = k = 1;
	while(k < numLines_s) {
		i = 1;
		while(i < numLines_c) {
			
			if(strcmp(s[k].file_name, c[i].file_name) == 0) {
				break;
			}			


				// Checks for REMOVE
			if( (strcmp(s[k].file_name, c[i].file_name) != 0) && (i == numLines_c-1) ) {

				fd = open(commitPath, O_APPEND | O_RDWR);
				char vn[5];
				bzero(vn, 5);	
				sprintf(vn, "%d", s[k].version_number);

				char toAdd[strlen(s[k].file_name) + strlen(s[k].file_hash) + 12];
				bzero(toAdd, strlen(s[k].file_name) + strlen(s[k].file_hash) + 12);	
				
				strcpy(toAdd, "D ");
				strcat(toAdd, vn);
				strcat(toAdd, " ");
				strcat(toAdd, s[k].file_name);
				strcat(toAdd, " ");
				strcat(toAdd, s[k].file_hash);
				strcat(toAdd, "\n");
		
				write(fd, toAdd, strlen(toAdd));
				
				printf("%s\n", toAdd);

				close(fd);
				check = 1;
			}

			++i;
		}
	
		++k;
	}

		// Bad case
	if(check < 0) {
		rmv = remove(commitPath);
		close(sockfd);
		free(ipAddress);	
		free(portS);
		free(s);
		free(lc);
		free(c);
		exit(1);
	}


		// rename .commit, compress and send to server, change name back to original.
	int ren = rename(commitPath, commitBuf);


		// tar -czf archive3.tar.gz test/commit-
	char commandSend[strlen(commitBuf) + 27];
	bzero(commandSend, (strlen(commitBuf) + 27));
	strcpy(commandSend, "tar -czf archive3.tar.gz ");
	strcat(commandSend, commitBuf);

	sys = system(commandSend);	



		// Send archives .commit to server
	
	fd = open("archive3.tar.gz", O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Error opening: archive3.tar.gz\n");
	}

	cp = lseek(fd, (size_t)0, SEEK_CUR);	
	fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);

	unsigned char sendCommit[(int)fileSize + 1];

	int rd = read(fd, sendCommit, (int)fileSize);
	if(rd < 0) {
		fprintf(stderr, "Error reading from: archive3.tar.gz\n");	
	}
	close(fd);
	sendCommit[(int)fileSize] = '\0';

		// Send compressed project back to client.
	write(sockfd, sendCommit, (int)fileSize);
	printf("Success: sent compressed .commit to server.\n");

		// Remove tar file from server.
	rmv = remove("archive3.tar.gz");
	if(rmv < 0) {
		fprintf(stderr, "Error removing: archive3.tar.gz\n");
	}
		// Remove server's .Manifest
	rmv = remove(".Manifest");
	if(rmv < 0) {
		fprintf(stderr, "Error removing: .Manifest");
	}


		// Rename .commit-# back to .commit
	ren = rename(commitBuf, commitPath);

	close(sockfd);
	free(ipAddress);	
	free(portS);
	free(s);
	free(lc);
	free(c);
}


	// 3.5 - Push
void push(char* projName) {
	
		// Check if client has .Update file. If so, then check if it has any 'M' codes.
	char updatePath[strlen(projName) + 9];
	strcpy(updatePath, projName);
	strcat(updatePath, "/.Update");
	int acc = access(updatePath, F_OK);
	if(acc >= 0) {
		int fd = open(updatePath, O_RDONLY);
		
		off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
		size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
		lseek(fd, cp, SEEK_SET);
		
		char updateBuf[(int)fileSize + 1];
		bzero(updateBuf, (int)fileSize + 1);

		int rfd = read(fd, updateBuf, (int)fileSize);
		close(fd);
		updateBuf[(int)fileSize] = '\0';
		
		int i = 0;
		int numLines = 0;
		while(i < (int)fileSize) {
			if(updateBuf[i] == '\n') {
				++numLines;
			}
			++i;
		}	
			
		int check = 0;
		if(numLines > 0) {

				// Struct for holding parsed .Update file.
			uFiles *f = malloc(numLines*sizeof(*f));
			int rtn = parseUpdate(f, projName, 1);
			if(rtn == -2) {
				fprintf(stderr, "Error opening .Update file.\n");
				free(f);
				exit(1);
			}
			
				// Makes sure there are no 'M' codes
			i = 0;
			while(i < numLines) {
				if(strcmp(f[i].code, "M") == 0) {
					fprintf(stdout, "File: `%s` was modified since the last upgrade. Please upgrade again before attempting to commit+push.\n", f[i].file_name);
					check = 1;
				}
				++i;
			}


			free(f);
		}

		if(check == 1) {
			printf("Exiting...\n");
			exit(1);
		}
	}


	char commitPath[strlen(projName) + 9];
	strcpy(commitPath, projName);
	strcat(commitPath, "/.commit");
		// Update file is fine, continue...

		// Check if client has .commit file
	printf("Accessing: %s\n", commitPath);
	acc = access(commitPath, F_OK);
	if(acc < 0) {
		fprintf(stdout, "No .commit file found in the client. Please do a `./WTF commit %s` before attempting to push.\n", projName);
		exit(1);
	} else {
		// .commit is there...see if it's empty. If so, up-to-date!
		int fd = open(commitPath, O_RDONLY);
		
		off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
		size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
		lseek(fd, cp, SEEK_SET);
		
		char testingBuf[(int)fileSize + 1];
		bzero(testingBuf, (int)fileSize + 1);

		int rfd = read(fd, testingBuf, (int)fileSize);
		close(fd);
		testingBuf[(int)fileSize] = '\0';
		
		int i = 0;
		int numLinesCheck = 0;
		while(i < (int)fileSize) {
			if(testingBuf[i] == '\n') {
				++numLinesCheck;
			}
			++i;
		}	
			
		if(numLinesCheck <= 0) {
			fprintf(stdout, "Commit file is empty. Project is up-to-date!\n");
			int rmvC = remove(commitPath);
			exit(1);
		}
		
	}	

	
		// Connect to server and retrieve .Manifest	
	int sockfd = -1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		fprintf(stderr, "Error creating server socket.\n");
		exit(1);
	}


	struct hostent *server;
	struct sockaddr_in serverAddr;	

		// Obtain IP Address and Port from .configure file
	char* ipAddress = getConfig(2);
	char* portS = getConfig(1);
	int portNum = atoi(portS);
	server = gethostbyname(getConfig(2));
   
	if (server == NULL) {
	  fprintf(stderr,"ERROR, no such host\n");
	  free(ipAddress);
	  free(portS);
	  exit(0);
	}
   
	bzero((char*)&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	//serverAddr.sin_addr.s_addr = inet_addr(ipAddress);
	bcopy((char *)server->h_addr, (char *)&serverAddr.sin_addr.s_addr, server->h_length);
	serverAddr.sin_port = htons(portNum);


	if( connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0 ) {
		fprintf(stderr, "Error connecting client to server.\n");
		free(ipAddress);
		free(portS);
		exit(1);
	} else {
		printf("Established connection to server.\n");
	}

		// Send project name to server.
		// update:  :<projName>
	int n, k, i, recLength;
	n = i = k = recLength = 0;
	
	char sendBuf[9 + strlen(projName)];
	bzero(sendBuf, 9 + strlen(projName));
	
	char pnBytes[2];
	snprintf(pnBytes, 10, "%d", strlen(projName));

	strcpy(sendBuf, "push:");
	strcat(sendBuf, pnBytes);
	strcat(sendBuf, ":");
	strcat(sendBuf, projName);

	n = write(sockfd, sendBuf, strlen(sendBuf));
	
        fd_set set;
        struct timeval timeout;

        FD_ZERO(&set);
        FD_SET(sockfd, &set);

        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        select(FD_SETSIZE, &set, NULL, NULL, &timeout);

        i = ioctl(sockfd, FIONREAD, &recLength);

        if(i < 0) {
                fprintf(stderr, "Error with ioctl().\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
                exit(1);
        }

        char recBuf[recLength+1];
        bzero(recBuf, recLength+1);

	if(recLength > 0) {
		n = read(sockfd, recBuf, recLength);
	}
	
	if(recLength == 0) {
		printf("Project name: `%s` doesn't exist on server.\n", projName);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}


		// Send commit with all files in it to the server.
	
		// Parse the .commit to get the files.
	int fd = open(commitPath, O_RDONLY);
	
	off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
	size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);
	
	char updateBuf[(int)fileSize + 1];
	bzero(updateBuf, (int)fileSize + 1);

	int rfd = read(fd, updateBuf, (int)fileSize);
	close(fd);
	updateBuf[(int)fileSize] = '\0';
	
	i = 0;
	int numLines = 0;
	while(i < (int)fileSize) {
		if(updateBuf[i] == '\n') {
			++numLines;
		}
		++i;
	}	
		

	uFiles *c = malloc(numLines*sizeof(*c));
	int rtn = parseUpdate(c, projName, 2);
	if(rtn == -2) {
		fprintf(stderr, "Error opening .Update file.\n");
		free(c);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}

	

	// tar -czf archive4.tar.gz 1 2 3
	int bytes = 0;
	i = 0;
	while(i < numLines) {
		bytes += strlen(c[i].file_name);
		++i;
	}

	char command[26 + (numLines*(bytes+2)) + (strlen(commitPath))];
	bzero(command, (26 + (numLines*(bytes+2)) + (strlen(commitPath))));

	strcpy(command, "tar -czf archive4.tar.gz ");

	i = 0;
	while(i < numLines) {
		if( (strcmp(c[i].code, "U") == 0) || (strcmp(c[i].code, "A") == 0) ) {
			strcat(command, c[i].file_name);
			strcat(command, " ");
		}
		++i;
	}

	strcat(command, commitPath);

	int sys = system(command);
	if(sys < 0) {
		fprintf(stderr, "Error tar-ing files from .commit.\n");
		free(c);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}

	fd = open("archive4.tar.gz", O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Error opening archive4.tar.gz.\n");	
		free(c);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}
	

	cp = lseek(fd, (size_t)0, SEEK_CUR);	
	fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);

	unsigned char sendTar[(int)fileSize + 1];
	
	int rd = read(fd, sendTar, (int)fileSize);
	if(rd < 0) {
		fprintf(stderr, "Error reading from archive4.tar.gz.\n");
		free(c);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}
	close(fd);	
	sendTar[(int)fileSize] = '\0';

	write(sockfd, sendTar, (int)fileSize);
	printf("Success: sent .commit and all required files to the server.\n");


		// Receive new .Manifest from the server
        FD_ZERO(&set);
        FD_SET(sockfd, &set);

        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        select(FD_SETSIZE, &set, NULL, NULL, &timeout);

        i = ioctl(sockfd, FIONREAD, &recLength);

        if(i < 0) {
                fprintf(stderr, "Error with ioctl().\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		free(c);
                exit(1);
        }

        char newM[recLength+1];
        bzero(newM, recLength+1);

	if(recLength > 0) {
		n = read(sockfd, newM, recLength);
	}

	printf("Received compressed .Manifest from the server. Swapping out...\n");
	
		// Take the compressed .Manifest from the server and replace it
		// with the .Manifest in the current project	
	fd = open("archive5.tar.gz", O_CREAT | O_RDWR, 0644);
	if(fd < 0) {
		fprintf(stderr, "Error creating archive5.tar.gz in client\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		free(c);
		exit(1);
		
	}
	write(fd, newM, recLength);
	close(fd);


		// untar project into current directory
	char untarM[46];
	strcpy(untarM, "tar --strip-components=1 -zxf archive5.tar.gz");
	sys = system(untarM);
	if(sys < 0) {
		fprintf(stderr, "Error untar-ing .Manifest received from server.\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
		free(c);
		exit(1);
	}

		// Remove unwanted things
	int rmv = remove("archive5.tar.gz");
	rmv = remove("archive4.tar.gz");
	rmv = remove(commitPath);

	
	free(c);
	close(sockfd);
	free(ipAddress);
	free(portS);
	
}


	// 
void history(char* projName) {

		// connect to server
	int sockfd = -1;
	int newsockfd = -1;
	int addrInfo = -1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		fprintf(stderr, "Error creating server socket.\n");
		exit(1);
	}


	struct hostent *server;
	struct sockaddr_in serverAddr;	

		// Obtain IP Address and Port from .configure file
	char* ipAddress = getConfig(2);
	char* portS = getConfig(1);
	int portNum = atoi(portS);
	server = gethostbyname(getConfig(2));
   
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		free(ipAddress);
		free(portS);
	 	exit(0);
	}
   
	bzero((char*)&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	//serverAddr.sin_addr.s_addr = inet_addr(ipAddress);
	bcopy((char *)server->h_addr, (char *)&serverAddr.sin_addr.s_addr, server->h_length);
	serverAddr.sin_port = htons(portNum);


	if( connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0 ) {
		fprintf(stderr, "Error connecting client to server.\n");
		free(ipAddress);
		free(portS);
		exit(1);
	} else {
		printf("Established connection to server.\n");
	}

		// Send project name to server.
		// update:  :<projName>
	int n, i, recLength;
	n = i = recLength = 0;
	
	char sendBuf[12 + strlen(projName)];
	bzero(sendBuf, 12 + strlen(projName));
	
	char pnBytes[2];
	snprintf(pnBytes, 10, "%d", strlen(projName));

	strcpy(sendBuf, "history:");
	strcat(sendBuf, pnBytes);
	strcat(sendBuf, ":");
	strcat(sendBuf, projName);

	n = write(sockfd, sendBuf, strlen(sendBuf));
	
        fd_set set;
        struct timeval timeout;

        FD_ZERO(&set);
        FD_SET(sockfd, &set);

        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        select(FD_SETSIZE, &set, NULL, NULL, &timeout);

        i = ioctl(sockfd, FIONREAD, &recLength);

        if(i < 0) {
                fprintf(stderr, "Error with ioctl().\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
                exit(1);
        }

        char recBuf[recLength+1];
        bzero(recBuf, recLength+1);

	if(recLength > 0) {
		n = read(sockfd, recBuf, recLength);
	}
	
	if(recLength == 0) {
		printf("Project name: `%s` doesn't exist on server.\n", projName);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}


	printf("History received from the server:\n%s\n", recBuf);
}


void rollback(char* projName, char* version) {
	
		// connect to server
	int sockfd = -1;
	int newsockfd = -1;
	int addrInfo = -1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		fprintf(stderr, "Error creating server socket.\n");
		exit(1);
	}


	struct hostent *server;
	struct sockaddr_in serverAddr;	

		// Obtain IP Address and Port from .configure file
	char* ipAddress = getConfig(2);
	char* portS = getConfig(1);
	int portNum = atoi(portS);
	server = gethostbyname(getConfig(2));
   
	if (server == NULL) {
	  fprintf(stderr,"ERROR, no such host\n");
	  exit(0);
	}
   
	bzero((char*)&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	//serverAddr.sin_addr.s_addr = inet_addr(ipAddress);
	bcopy((char *)server->h_addr, (char *)&serverAddr.sin_addr.s_addr, server->h_length);
	serverAddr.sin_port = htons(portNum);


	if( connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0 ) {
		fprintf(stderr, "Error connecting client to server.\n");
		free(ipAddress);
		free(portS);
		exit(1);
	} else {
		printf("Established connection to server.\n");
	}

		// Send project name to server.
		// update:  :<projName>
	int n, i, recLength;
	n = i = recLength = 0;
	
	char sendBuf[17 + strlen(projName)];
	bzero(sendBuf, 17 + strlen(projName));
	
	char pnBytes[2];
	snprintf(pnBytes, 10, "%d", strlen(projName));

	strcpy(sendBuf, "rollback:");
	strcat(sendBuf, pnBytes);
	strcat(sendBuf, ":");
	strcat(sendBuf, projName);
	strcat(sendBuf, ":");
	strcat(sendBuf, version);

	n = write(sockfd, sendBuf, strlen(sendBuf));
	
        fd_set set;
        struct timeval timeout;

        FD_ZERO(&set);
        FD_SET(sockfd, &set);

        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        select(FD_SETSIZE, &set, NULL, NULL, &timeout);

        i = ioctl(sockfd, FIONREAD, &recLength);

        if(i < 0) {
                fprintf(stderr, "Error with ioctl().\n");
		close(sockfd);
		free(ipAddress);
		free(portS);
                exit(1);
        }

        char recBuf[recLength+1];
        bzero(recBuf, recLength+1);

	if(recLength > 0) {
		n = read(sockfd, recBuf, recLength);
	}
	
	if(recLength == 0) {
		printf("Rollback failed\n", projName);
		close(sockfd);
		free(ipAddress);
		free(portS);
		exit(1);
	}

	printf("Rollback successful.\n");	
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
			fprintf(stderr, "Invalid number of arguments for ADD/REMOVE.\nExpected 2.\nReceived %d\n", argc-2);
			exit(1);
		} else {
			if(strcmp(argv[1], "add") == 0) {
				add_or_remove(1, argv[2], argv[3]);
			} else {
				add_or_remove(2, argv[2], argv[3]);
			}
		}

	} else if(strcmp(argv[1], "checkout") == 0) {

		if(argc != 3) {
			fprintf(stderr, "Invalid number of arguments for CHECKOUT.\nExpected 1.\nReceived %d\n", argc-2);
			exit(1);
		} else {
			resolveIP();
			checkout(argv[2]);
		}
	} else if(strcmp(argv[1], "currentversion") == 0) {

		if(argc != 3) {

			fprintf(stderr, "Invalid number of arguments for CURRENTVERSION.\nExpected 1.\nReceived %d\n", argc-2);
			exit(1);
		} else {
			resolveIP();
			currentVersion(argv[2]);
		}
	} else if(strcmp(argv[1], "update") == 0) {

		if(argc != 3) {

			fprintf(stderr, "Invalid number of arguments for UPDATE.\nExpected 1.\nReceived %d\n", argc-2);
			exit(1);
		} else {
			resolveIP();
			update(argv[2]);
		}

	} else if(strcmp(argv[1], "upgrade") == 0) {

		if(argc != 3) {

			fprintf(stderr, "Invalid number of arguments for UPGRADE.\nExpected 1.\nReceived %d\n", argc-2);
			exit(1);
		} else {
			resolveIP();
			upgrade(argv[2]);
		}

	} else if(strcmp(argv[1], "commit") == 0) {

		if(argc != 3) {

			fprintf(stderr, "Invalid number of arguments for COMMIT.\nExpected 1.\nReceived %d\n", argc-2);
			exit(1);
		} else {
			resolveIP();
			commit(argv[2]);
		}

	} else if(strcmp(argv[1], "push") == 0) {
		
		if(argc != 3) {

			fprintf(stderr, "Invalid number of arguments for PUSH.\nExpected 1.\nReceived %d\n", argc-2);
			exit(1);
		} else {
			resolveIP();
			push(argv[2]);
		}
	} else if(strcmp(argv[1], "history") == 0) {

		if(argc != 3) {

			fprintf(stderr, "Invalid number of arguments for HISTORY.\nExpected 1.\nReceived %d\n", argc-2);
			exit(1);
		} else {
			resolveIP();
			history(argv[2]);
		}

	} else if(strcmp(argv[1], "rollback") == 0) {

		if(argc != 4) {

			fprintf(stderr, "Invalid number of arguments for ROLLBACK.\nExpected 1.\nReceived %d\n", argc-2);
			exit(1);
		} else {
			resolveIP();
			rollback(argv[2], argv[3]);
		}

	} else if(strcmp(argv[1], "destroy") == 0) {

		if(argc != 3) {

			fprintf(stderr, "Invalid number of arguments for DESTROY.\nExpected 1.\nReceived %d\n", argc-2);
			exit(1);
		} else {
			resolveIP();
			destroy(argv[2]);
		}


	} else {
		printf("invalid command\n");
	}


	return 0;
}
