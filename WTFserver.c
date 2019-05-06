#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include <openssl/sha.h>
#include "helper.h"

struct args {
    char* str;
    int num;
};
	// Gets the command given from the protocol.
	// 1 - create
	// 2 - destroy
	// 3 - checkout
	// 4 - currentversion
	// 5 - update
	// 6 - upgrade
	// 7 - commit
	// 8 - push
	// 9 - history
	// 10 - rollback
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
		printf("Received CREATE command from client.\n");
		return 1;
	} else if(strcmp(command, "destroy") == 0) {
		printf("Received DESTROY command from client.\n");
		return 2;
	} else if(strcmp(command, "checkout") == 0) {
		printf("Received CHECKOUT command from client.\n");
		return 3;
	} else if(strcmp(command, "currentversion") == 0) {
		printf("Received CURRENTVERSION command from client.\n");
		return 4;
	} else if(strcmp(command, "update") == 0) {
		printf("Received UPDATE command from client.\n");
		return 5;
	} else if(strcmp(command, "upgrade") == 0) {
		printf("Received UPGRADE command from client.\n");
		return 6;
	} else if(strcmp(command, "commit") == 0) {
		printf("Received COMMIT command from client.\n");
		return 7;
	} else if(strcmp(command, "push") == 0) {
		printf("Received PUSH command from client.\n");
		return 8;
	} else if(strcmp(command, "history") == 0) {
		printf("Received HISTORY command from client.\n");
		return 9;
	} else if(strcmp(command, "rollback") == 0) {
		printf("Received ROLLBACK command from client.\n");
		return 10;
	}  else {
		;
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
void parseManifest(files* f, char* projName) {

	
	char pathManifest[strlen(projName) + 20];
	strcpy(pathManifest, ".server/");
	strcat(pathManifest, projName);
	strcat(pathManifest, "/.Manifest");



	int fd = open(pathManifest, O_RDONLY);		
	if(fd < 0) {
		printf("Error opening file: %s\n", pathManifest);
		exit(1);
	}		

	off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
	size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);

	char buffer[(int)fileSize + 1];

	int rfd = read(fd, buffer, (int)fileSize);
	if(rfd < 0) {
		fprintf(stderr, "Error readiing from file: %s", pathManifest);
		exit(1);
	}
	close(fd);

	int numElements, line, i;
	i = numElements = line = 0;
	
	while(buffer[i] != '\n') {
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

	if(strlen(buffer)-i <= 0)  {
		return;
	}

	char newTemp[strlen(buffer)-i];
	memcpy(newTemp, buffer+i, strlen(buffer)-i);

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

	char* tokenized[numElements+1];
	i = 0;
	tokenized[i] = strtok(newTemp, " ,");
	
	while(tokenized[i] != NULL) {
		tokenized[++i] = strtok(NULL, " ,");
	}

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


	char pathUpdate[strlen(projName) + 17];
	strcpy(pathUpdate, ".server/");
	strcat(pathUpdate, projName);

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

	// 3.6 -- create
void *create(void *input) {
	char* projName = ((struct args*)input)->str;
	int sockfd = ((struct args*)input)->num;
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
						return;
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
	
        int fd = open(newDir, O_CREAT | O_RDWR | O_APPEND, 0644);

        if(fd < 0) {
                fprintf(stderr, "Configure never ran. Please run:\n./WTF configure <IP> <PORT>\n");
                return;
        }

	write(fd, "1\n", 2);
	close(fd);

		// Creates the version folder
	char vPath[strlen(projName) + 19];
	strcpy(vPath, ".server/");
	strcat(vPath, projName);
	strcat(vPath, "/.versions");
	mkdir(vPath, 0700);
		
		// Created the history file
	char hPath[strlen(projName) + 18];
	strcpy(hPath, ".server/");
	strcat(hPath, projName);
	strcat(hPath, "/.history");
	///*
	//create\n0
	char writeH[16];
	bzero(writeH, 16);
	strcpy(writeH, "create\n");
	strcat(writeH, "0\n\n");
	fd = open(hPath, O_CREAT | O_RDWR, 0644);

	write(fd, writeH, strlen(writeH));
	
	close(fd);
	
	char sendBuf[2];
	sendBuf[0] = '1';
	sendBuf[1] = '\n';

	write(sockfd, sendBuf, 2);
	return;
}

	// Used for recurssion. Concats directory strings.
char* concatDir(char* original, char* toAdd) {

	int bufferSize = strlen(original) + strlen(toAdd) + 2;
	
	char* concat = malloc(bufferSize);
	strcpy(concat, original);
	strcat(concat, "/");
	strcat(concat, toAdd);

	return concat;
}

	// Used for rollback
void rollbackDelete(char* pPath) {

	int rmv = 0;
	DIR *d = opendir(pPath);	
	struct dirent* status = NULL;
	char* newDir;	

		if( d != NULL ) {

			status = readdir(d);

			do {
			
				char* fullDir = concatDir(pPath, status->d_name);	

				if( status->d_type == DT_REG ) {
					if(strcmp(status->d_name, ".history") != 0) {
						rmv = remove(fullDir);
					}
				} else {
					;
				}

					// Recurssive method for going through subdirectories	
				if( status->d_type == DT_DIR ) {
					if( (strcmp(status->d_name, ".") == 0) || (strcmp(status->d_name, "..") == 0) || (strcmp(status->d_name, ".versions") == 0) ) {
						;
					} else {
						newDir = concatDir(pPath, status->d_name);
						rollbackDelete(newDir);
						rmv = rmdir(newDir);
						free(newDir);
					}
				}
				free(fullDir);				
				status = readdir(d);
			} while ( status != NULL );
		closedir(d);
		} 
}

	// Used for destroy
void recursiveDelete(char* pPath) {

	int rmv = 0;
	DIR *d = opendir(pPath);	
	struct dirent* status = NULL;
	char* newDir;	

		if( d != NULL ) {

			status = readdir(d);

			do {
			
				char* fullDir = concatDir(pPath, status->d_name);	

				if( status->d_type == DT_REG ) {
					rmv = remove(fullDir);
				} else {
					;
				}

					// Recurssive method for going through subdirectories	
				if( status->d_type == DT_DIR ) {
					if( (strcmp(status->d_name, ".") == 0) || (strcmp(status->d_name, "..") == 0) ) {
						;
					} else {
						newDir = concatDir(pPath, status->d_name);
						recursiveDelete(newDir);
						rmv = rmdir(newDir);
						free(newDir);
					}
				}
				free(fullDir);				
				status = readdir(d);
			} while ( status != NULL );
		closedir(d);
		} 
}


void *destroy(void *input) {
	char* projName = ((struct args*)input)->str;
	int sockfd = ((struct args*)input)->num;
		// Will be set to 1 if projName exists
	int checkExist = 0;
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
						checkExist = 1;
						break;
					}
				}
			}
			status = readdir(d);
		} while(status != NULL);
		closedir(d);
	}

		// Project existence check
	if(checkExist == 0) {
		fprintf(stderr, "Error: Project does not exists on the server.\n");
		return;
	}
	

	char pPath[strlen(projName) + 9];
	strcpy(pPath, ".server/");
	strcat(pPath, projName);

	recursiveDelete(pPath);
	int rmv = rmdir(pPath);
	if(rmv < 0) {
		fprintf(stderr, "Error removing project directory: %s\n.", pPath);
		return;
	}

	write(sockfd, "Success", 7);
}


void *checkout(void *input) {
	
	char* projName = ((struct args*)input)->str;
	int sockfd = ((struct args*)input)->num;
		// Will be set to 1 if projName exists
	int checkExist = 0;
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
						checkExist = 1;
						break;
					}
				}
			}
			status = readdir(d);
		} while(status != NULL);
		closedir(d);
	}

		// Project existence check
	if(checkExist == 0) {
		fprintf(stderr, "Error: Project does not exists on the server.\n");
		return;
	}

		// Creates the version folder
	char vPath[strlen(projName) + 19];
	strcpy(vPath, ".server/");
	strcat(vPath, projName);
	strcat(vPath, "/.versions");
	mkdir(vPath, 0700);
		
		// Created the history file
	char hPath[strlen(projName) + 18];
	strcpy(hPath, ".server/");
	strcat(hPath, projName);
	strcat(hPath, "/.history");
	


	char command[strlen(projName) + strlen(vPath) + strlen(hPath) + 71];
	bzero(command, (strlen(projName) + strlen(vPath) + strlen(hPath) + 71));
	strcpy(command, "tar -czf .server/archive.tar.gz ");
	strcat(command, "--exclude=\"");
	strcat(command, vPath);
	strcat(command, "\" --exclude=\"");
	strcat(command, hPath);
	strcat(command, "\" .server/");
	strcat(command, projName);

		// Tar project and send it over.
	int sys = system(command);
	if(sys < 0) {
		fprintf(stderr, "Error compressing project: %s\n", projName);
		return;
	}

	int fd = open(".server/archive.tar.gz", O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Error opening: .server/archive.tar.gz\n");
		return;
	}

	off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
	size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);

	unsigned char buffer[(int)fileSize + 1];

	int rd = read(fd, buffer, (int)fileSize);
	if(rd < 0) {
		fprintf(stderr, "Error reading from: .server/archive.tar.gz\n");	
		return;
	}
	close(fd);

		// Send compressed project back to client.
	write(sockfd, buffer, (int)fileSize);
	printf("Sent compressed project to client.\n");

		// Remove tar file from server.
	int rmv = remove(".server/archive.tar.gz");
	if(rmv < 0) {
		fprintf(stderr, "Error removing: .server/archive.tar.gz\n");
		return;
	}
	return;
}

void *currentversion(void *input) {
	
	char* projName = ((struct args*)input)->str;
	int sockfd = ((struct args*)input)->num;
		// Will be set to 1 if projName exists
	int checkExist = 0;
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
						checkExist = 1;
						break;
					}
				}
			}
			status = readdir(d);
		} while(status != NULL);
		closedir(d);
	}

		// Project existence check
	if(checkExist == 0) {
		fprintf(stderr, "Error: Project does not exists on the server.\n");
		return;
	}

		// Project exists, parse and send manifest info to client

	char pathManifest[strlen(projName) + 20];
	strcpy(pathManifest, ".server/");
	strcat(pathManifest, projName);
	strcat(pathManifest, "/.Manifest");

	int fd = open(pathManifest, O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Error opening file: %s\n", pathManifest);
		return;
	}

	off_t cp = lseek(fd, (size_t)0, SEEK_CUR);
	size_t fileSize = lseek(fd, (size_t)0, SEEK_END);
	lseek(fd, cp, SEEK_SET);

	char manifestBuf[(int)fileSize + 1];

	int rfd = read(fd, manifestBuf, (int)fileSize);
	if(rfd < 0) {
		fprintf(stderr, "error reading from file: %s\n", pathManifest);
		return;	
	}
	close(fd);

		// Gets number of lines from .Manifest
	int i = 0;
	int numLines = 0;
	while(i < (int)fileSize) {
		if(manifestBuf[i] == '\n') {
			++numLines;
		}
		++i;
	}

		// Create files struct for .Manifest
	files *f = malloc(numLines * sizeof(*f));
	parseManifest(f, projName);
	
	char buffer[numLines*sizeof(*f)];
	char vn[5];

	i = 0;
	while(i < numLines) {
		sprintf(vn, "%d", f[i].version_number);
		if(i == 0) {
			strcpy(buffer, f[i].file_name);
			strcat(buffer, " : ");
			strcat(buffer, vn);
			strcat(buffer, "\n");
			bzero(vn, 5);
		} else {
			strcat(buffer, f[i].file_name);
			strcat(buffer, " : ");
			strcat(buffer, vn);
			strcat(buffer, "\n");
			bzero(vn, 5);
		}
		++i;
	}


		// Send to cliient	
	write(sockfd, buffer, strlen(buffer));
	printf("Current versions successfully sent to the client.\n");

	free(f);
	return;	

}

void *update(void *input) {
	
	char* projName = ((struct args*)input)->str;
	int sockfd = ((struct args*)input)->num;
		// Will be set to 1 if projName exists
	int checkExist = 0;
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
						checkExist = 1;
						break;
					}
				}
			}
			status = readdir(d);
		} while(status != NULL);
		closedir(d);
	}

		// Project existence check
	if(checkExist == 0) {
		fprintf(stderr, "Error: Project does not exists on the server.\n");
		return;
	}


	char command[strlen(projName) + 52];
	bzero(command, (strlen(projName) + 52));
	strcpy(command, "tar -czf .server/archive.tar.gz .server/");
	strcat(command, projName);
	strcat(command, "/.Manifest");

		// Tar project and send it over.
	int sys = system(command);
	if(sys < 0) {
		fprintf(stderr, "Error compressing `.Manifest` in project: %s\n", projName);
		return;
	}

	int fd = open(".server/archive.tar.gz", O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Error opening: .server/archive.tar.gz\n");
		return;
	}

	off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
	size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);

	unsigned char buffer[(int)fileSize + 1];

	int rd = read(fd, buffer, (int)fileSize);
	if(rd < 0) {
		fprintf(stderr, "Error reading from: .server/archive.tar.gz\n");	
		return;
	}
	close(fd);

		// Send compressed project back to client.
	write(sockfd, buffer, (int)fileSize);
	printf("Sent compressed .Manifest to client for evaluation.\n");

		// Remove tar file from server.
	int rmv = remove(".server/archive.tar.gz");
	if(rmv < 0) {
		fprintf(stderr, "Error removing: .server/archive.tar.gz\n");
		return;
	}

	return;	
}


void *upgrade(void *input) {
	
	char* projName = ((struct args*)input)->str;
	int sockfd = ((struct args*)input)->num;
		// Will be set to 1 if projName exists
	int checkExist = 0;
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
						checkExist = 1;
						break;
					}
				}
			}
			status = readdir(d);
		} while(status != NULL);
		closedir(d);
	}

		// Project existence check
	if(checkExist == 0) {
		fprintf(stderr, "Error: Project does not exists on the server.\n");
		return;
	}

	int n = 0;
	fd_set set;
	struct timeval timeout;

	FD_ZERO(&set);
	FD_SET(sockfd, &set);

	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	//ensures something is bring read from client
	select(FD_SETSIZE, &set, NULL, NULL, &timeout);
	//n is length of buffer
	int i = ioctl(sockfd, FIONREAD, &n);
	
	if(i < 0) {
		fprintf(stderr, "Error with ioctl().\n");
		return;
	}
	
	char *buffer = malloc((n+1)*sizeof(char)); //[n+1];
	bzero(buffer, n+1);

	if(n > 0) {
		n = read(sockfd, buffer, n);
	}
	
	if(strcmp(buffer, "") == 0) {
		fprintf(stdout, "Client's project up-to-date. No need to continue. Exiting...\n");
		return;
	}


		// tar -czf .server/archive1.tar.gz .server/projName/1 ./server/projName/2 
		// Take given file names from the client and reformat into names used for tar.
	int numLines = 0;
	i = 0;
	while(i < strlen(buffer)) {
		if(buffer[i] == ' ') {
			++numLines;
		}
		++i;
	}

	char tarBuf[numLines*(11 + strlen(projName))];
	bzero(tarBuf, (numLines*(11 + strlen(projName))));
	
		//.server/ / 
	char prefix[9];
	bzero(prefix, 9);
	
	strcpy(prefix, ".server/");
	
//	printf("%s\n", buffer);

	i = 0;
	int k = 0;
	while(i < strlen(buffer)) {
		k = i;
		while((k < strlen(buffer)) && (buffer[k] != ' ')) {
			++k;
		}

		int length = k - i;
		char temp[length + 1];
		bzero(temp, (length+1));
		
		memcpy(temp, buffer+i, length);
		temp[length] = '\0';

		strcat(tarBuf, prefix);
		strcat(tarBuf, temp);
		strcat(tarBuf, " ");


//		printf("%s\n", buffer);		
		i = k;
		++i;
	}
	
//	printf("%s\n", tarBuf);

		// contains .Update files...do stuff with M, A
	char command[strlen(tarBuf) + 34];
	bzero(command, (strlen(tarBuf) + 34));

	strcpy(command, "tar -czf .server/archive1.tar.gz ");
	strcat(command, tarBuf);


	int sys = system(command);
	if(sys < 0) {
		fprintf(stderr, "Error compressing archive. Most likely a file that is suppose to be tar'd does not exists.\n");
		free(buffer);
		return;
	}

	int fd = open(".server/archive1.tar.gz", O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Error opening: .server/archive1.tar.gz\n");
		free(buffer);
		return;
	}

	off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
	size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);

	unsigned char sendBuf[(int)fileSize + 1];

	int rd = read(fd, sendBuf, (int)fileSize);
	if(rd < 0) {
		fprintf(stderr, "Error reading from: .server/archive1.tar.gz\n");	
		free(buffer);
		return;
	}
	close(fd);
	sendBuf[(int)fileSize] = '\0';

		// Send compressed project back to client.
	write(sockfd, sendBuf, (int)fileSize);
	printf("Sent compressed files to client.\n");

		// Remove tar file from server.
	int rmv = remove(".server/archive1.tar.gz");
	if(rmv < 0) {
		fprintf(stderr, "Error removing: .server/archive1.tar.gz\n");
		free(buffer);
		return;
	}

	free(buffer);
	return;
}

void *commit(void *input) {
	
	char* projName = ((struct args*)input)->str;
	int sockfd = ((struct args*)input)->num;
		// Will be set to 1 if projName exists
	int checkExist = 0;
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
						checkExist = 1;
						break;
					}
				}
			}
			status = readdir(d);
		} while(status != NULL);
		closedir(d);
	}

		// Project existence check
	if(checkExist == 0) {
		fprintf(stderr, "Error: Project does not exists on the server.\n");
		return;
	}

	char command[strlen(projName) + 52];
	bzero(command, (strlen(projName) + 52));
	strcpy(command, "tar -czf .server/archive.tar.gz .server/");
	strcat(command, projName);
	strcat(command, "/.Manifest");
	

		// Tar project and send it over.
	int sys = system(command);
	if(sys < 0) {
		fprintf(stderr, "Error compressing `.Manifest` in project: %s\n", projName);
		return;
	}

	int fd = open(".server/archive.tar.gz", O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Error opening: .server/archive.tar.gz\n");
		return;
	}

	off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
	size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);

	unsigned char buffer[(int)fileSize + 1];

	int rd = read(fd, buffer, (int)fileSize);
	if(rd < 0) {
		fprintf(stderr, "Error reading from: .server/archive.tar.gz\n");	
		return;
	}
	close(fd);


		// Send compressed project back to client.
	write(sockfd, buffer, (int)fileSize);
	printf("Sent compressed .Manifest to client for evaluation.\n");

		// Remove tar file from server.
	int rmv = remove(".server/archive.tar.gz");
	if(rmv < 0) {
		fprintf(stderr, "Error removing: .server/archive.tar.gz\n");
		return;
	}



		// read .commit file from client and store as active commit 
	int n = 0;
	fd_set set;
	struct timeval timeout;

	FD_ZERO(&set);
	FD_SET(sockfd, &set);

	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	//ensures something is bring read from client
	select(FD_SETSIZE, &set, NULL, NULL, &timeout);
	//n is length of buffer
	int i = ioctl(sockfd, FIONREAD, &n);
	
	if(i < 0) {
		fprintf(stderr, "Error with ioctl().\n");
		return;
	}
	
	char *fileBuffer = malloc((n+1)*sizeof(char)); //[n+1];
	bzero(buffer, n+1);

	if(n > 0) {
		n = read(sockfd, fileBuffer, n);
	}

	if(n == 0) {
		fprintf(stdout, "Client needs to update+upgrade before commit+push.\n");
		free(fileBuffer);
		return;
	}
	
	printf("Received compressed .commit file from client. Storing as active commit.\n");

	fd = open(".server/archive3.tar.gz", O_CREAT | O_RDWR, 0644);
	write(fd, fileBuffer, n);	
	close(fd);

		// Untar .commit into project direcory.
		// tar -zxf archiv
	char untar[44];
	strcpy(untar, "tar -C .server -zxf .server/archive3.tar.gz");

	sys = system(untar);
	if(sys < 0) {
		fprintf(stderr, "Error un-tarring compressed .commit file.\n");
		free(fileBuffer);
		return;
	}

		// remove tar archive from server
	rmv = remove(".server/archive3.tar.gz");

	free(fileBuffer);
	return;	
}

void *push(void *input) {
	
	char* projName = ((struct args*)input)->str;
	int sockfd = ((struct args*)input)->num;
	
	char* pName = malloc(100*sizeof(char));
	strcpy(pName, projName);


		// Will be set to 1 if projName exists
	int checkExist = 0;
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
						checkExist = 1;
						break;
					}
				}
			}
			status = readdir(d);
		} while(status != NULL);
		closedir(d);
	}

		// Project existence check.
		// Send to client yes (project exists) or no (doesn't exist)
	if(checkExist == 0) {
		fprintf(stderr, "Error: Project does not exists on the server.\n");
		free(pName);
		return;
	} else {
		write(sockfd, "yes", 3);
	}

	int n = 0;
	fd_set set;
	struct timeval timeout;

	FD_ZERO(&set);
	FD_SET(sockfd, &set);

	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
		//ensures something is being read from client
	select(FD_SETSIZE, &set, NULL, NULL, &timeout);



		//n is length of buffer
	int i = ioctl(sockfd, FIONREAD, &n);
	
	if(i < 0) {
		fprintf(stderr, "Error with ioctl().\n");
		free(pName);
		return;
	}
	char *buffer = malloc((n+1)*sizeof(char)); //[n+1];

	if(n > 0) {
		n = read(sockfd, buffer, n);
	}
	if(strcmp(buffer, "") == 0) {
		fprintf(stdout, "Client's project up-to-date. No need to continue. Exiting...\n");
		free(pName);
		free(buffer);
		return;
	}

		// Creates archive for the tar sent by the client.
	int fd = open(".server/archive4.tar.gz", O_CREAT | O_RDWR, 0644);
	if(fd < 0) {
		fprintf(stderr, "Error creating archive4.tar.gz.\n");
		free(buffer);
		free(pName);
		return;
	}

	write(fd, buffer, n);
	close(fd);

		// Untar only the .commit file to check if there is an active commit that matches it.
		// tar -C .server -zxf .server/archive4.tar.gz test/.commit
	char unTar[strlen(pName) + 53];
	strcpy(unTar, "tar -C .server -zxf .server/archive4.tar.gz ");
	strcat(unTar, projName);
	strcat(unTar, "/.commit");
	int sys = system(unTar);
	if(sys < 0) {
		fprintf(stderr, "Error extracting .commit from archive4.tar.gz.\n");
		free(buffer);
		free(pName);
		return;
	}

		//.server/ /.commit
	char c_commit[strlen(pName) + 17];
	strcpy(c_commit, ".server/");
	strcat(c_commit, pName);
	strcat(c_commit, "/.commit");

		// Get hash from .commit send by the client.
	fd = open(c_commit, O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Error opening file:.\nThis file is in the client's .Manifest but not in the project.\n");
		free(buffer);
		free(pName);
		return;
	}

	off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
	size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);

		// Generate hash for file.
	char bufferH[(int)fileSize+1];
	bzero(bufferH, (int)fileSize+1);
	int rfd = read(fd, bufferH, fileSize);
	if(rfd < 0) {
		fprintf(stderr, "Error reading from file: .commit");
		free(buffer);
		free(pName);
		return;
	}
	close(fd);
	bufferH[(int)fileSize] = '\0';

		// Generate live hash
	unsigned char hash[SHA256_DIGEST_LENGTH];
	bzero(hash, SHA256_DIGEST_LENGTH);
	SHA256(bufferH, strlen(bufferH), hash);
	
		// Converts hash into string.
	char hashStringC[65];
	bzero(hashStringC, 65);
	hashStringC[64] = '\0';
	int p = 0;
	while(p < SHA256_DIGEST_LENGTH) {
		sprintf(&hashStringC[p*2], "%02x", hash[p]);
		++p;
	}
	

		// .commit extracted. Need to see if there is an active commit that matches it.
		// If not, we can simply stop.
	char projectPath[strlen(pName) + 9];
	strcpy(projectPath, ".server/");
	strcat(projectPath, pName);

	
	d = opendir(projectPath);
	status = NULL;
	
	int k, check;
	check = k = 0;
	if(d != NULL) {
		
		status = readdir(d);

		do {
			if( status->d_type == DT_REG ) { 
				
					// See if there is an active .commit that matches the one from the client
					// Do this by computing hash and comparing.
				char dPath[strlen(pName) + strlen(status->d_name) + 10];
				bzero(dPath, (strlen(pName) + strlen(status->d_name) + 10));
				strcpy(dPath, ".server/");
				strcat(dPath, pName);
				strcat(dPath, "/");
				strcat(dPath, status->d_name);

				fd = open(dPath, O_RDONLY);
				if(fd < 0) {
					fprintf(stderr, "Error opening file:.\nThis file is in the client's .Manifest but not in the project.\n");
					free(buffer);
					free(pName);
					return;
				}

				cp = lseek(fd, (size_t)0, SEEK_CUR);	
				fileSize = lseek(fd, (size_t)0, SEEK_END); 
				lseek(fd, cp, SEEK_SET);

					// Generate hash for file.
				char bufferS[(int)fileSize+1];
				bzero(bufferS, (int)fileSize+1);
				rfd = read(fd, bufferS, fileSize);
				if(rfd < 0) {
					fprintf(stderr, "Error reading from file: .commit");
					free(buffer);
					free(pName);
					return;
				}
				close(fd);
				bufferS[(int)fileSize] = '\0';

					// Generate live hash
				unsigned char hashS[SHA256_DIGEST_LENGTH];
				bzero(hashS, SHA256_DIGEST_LENGTH);
				SHA256(bufferS, strlen(bufferS), hashS);
				
					// Converts hash into string.
				char hashStringS[65];
				bzero(hashStringS, 65);
				hashStringS[64] = '\0';
				p = 0;
				while(p < SHA256_DIGEST_LENGTH) {
					sprintf(&hashStringS[p*2], "%02x", hashS[p]);
					++p;
				}
				
					// See if two files have the same hash...
				if(strcmp(hashStringS, hashStringC) == 0) {
					if(strcmp(status->d_name, ".commit") != 0) {
						check = 1;
						break;
					}
				}


			}
			status = readdir(d);
		} while(status != NULL);
		closedir(d);
	}


		// If no matching active commit (expired commit) then remove archive and commit file
	if(check != 1) {
		printf("No matching active commit found on the server. Expired commit.\n");
		int rmv = remove(".server/archive4.tar.gz");
		rmv = remove(c_commit);
		free(buffer);
		free(pName);
		return;
	}	


		// Remove all other active commits...
	d = opendir(projectPath);
	status = NULL;
	
	if(d != NULL) {
		
		status = readdir(d);

		do {
			if( status->d_type == DT_REG ) { 

				char splitC[8];
				memcpy(splitC, status->d_name, 7);
				splitC[7] = '\0';
				
				if(strcmp(splitC, ".commit") == 0) {

					char delPath[strlen(pName) + strlen(status->d_name) + 10];
					bzero(delPath, (strlen(pName) + strlen(status->d_name) + 10));
					strcpy(delPath, ".server/");
					strcat(delPath, pName);
					strcat(delPath, "/");
					strcat(delPath, status->d_name);


					
						// Deletes all active commits	
					int rmvv = remove(delPath);
				}

			}
			status = readdir(d);
		} while(status != NULL);
		closedir(d);
	}


		// Look into the .versions folder and get the highest integer.
	char vPath[strlen(pName) + 19];
	strcpy(vPath, ".server/");
	strcat(vPath, pName);
	strcat(vPath, "/.versions");

	d = opendir(vPath);
	status = NULL;
	
	int highestVersion = 0;
	if(d != NULL) {
		
		status = readdir(d);

		do {
			if( status->d_type == DT_REG ) { 

				i = 0;
				while(status->d_name[i] != '.') {
					++i;
				}
				char temp[i+1];
				bzero(temp, i+1);
				memcpy(temp, status->d_name, i);
				temp[i] = '\0';
				
				int curVersion = atoi(temp);

				if(curVersion > highestVersion) {
					highestVersion = curVersion;
				}


			}
			status = readdir(d);
		} while(status != NULL);
		closedir(d);
	}


		// Will be the new latest version of the project
	highestVersion += 1;

		// Tar the current project and send copy it into .versions.
	char versionName[12];
	sprintf(versionName, "%d", highestVersion);
	strcat(versionName, ".tar.gz");	

	char to_Loc[strlen(vPath) + strlen(versionName) + 2];
	strcpy(to_Loc, vPath);
	strcat(to_Loc, "/");
	strcat(to_Loc, versionName);

	char historyPath[strlen(pName) + 18];
	strcpy(historyPath, ".server/");
	strcat(historyPath, pName);
	strcat(historyPath, "/.history");

	char dirC[strlen(projName) + 9];
	strcpy(dirC, ".server/");
	strcat(dirC, pName);

	// tar -czf .server/test/.versions/6.tar.gz --exclude=".server/test/.versions" --exclude=".server/test/.history" .server/test

	char tarV[strlen(to_Loc) + strlen(vPath) + strlen(historyPath) + strlen(dirC) + 41];
	bzero(tarV, (strlen(to_Loc) + strlen(vPath) + strlen(historyPath) + strlen(dirC) + 41));
	strcpy(tarV, "tar -czf ");
	strcat(tarV, to_Loc);
	strcat(tarV, " --exclude=\"");
	strcat(tarV, vPath);
	strcat(tarV, "\" --exclude=\"");
	strcat(tarV, historyPath);
	strcat(tarV, "\" ");
	strcat(tarV, dirC);
	
		// Compresses the project directory before changing/adding files to it. (extra credit)
	sys = system(tarV);
	if(sys < 0) {
		fprintf(stderr, "Error while compressing direction to store into .versions.\n");
		free(buffer);
		free(pName);
		return;
	}


		// tar -C .server -zxf .server/archive4.tar.gz
		// Directory is now compressed. We can continue to untar files from client into current directory.	
	char pushCommand[44];
	strcpy(pushCommand, "tar -C .server -zxf .server/archive4.tar.gz");

		// Untar the compressed files from the client into the servers version of the project.
	sys = system(pushCommand);
	if(sys < 0) {
		fprintf(stderr, "Error while decompressing files from client into the server's directory.\n");
		free(buffer);
		free(pName);
		return;
	}

		// Remove archive
	int rmv = remove(".server/archive4.tar.gz");


		// Update the .history and .Manifest on the server with files from .commit
		// Parse the .commit && the .Maniefst -> Do changes -> Rebuild .Manifest
	fd = open(c_commit, O_RDONLY);
	
	cp = lseek(fd, (size_t)0, SEEK_CUR);	
	fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);
	
	char commitBuf[(int)fileSize + 1];
	bzero(commitBuf, (int)fileSize + 1);

	rfd = read(fd, commitBuf, (int)fileSize);
	close(fd);
	commitBuf[(int)fileSize] = '\0';
	
	i = 0;
	int numLines_c = 0;
	while(i < (int)fileSize) {
		if(commitBuf[i] == '\n') {
			++numLines_c;
		}
		++i;
	}	
		
		// Struct for holding parsed .Update file.
	uFiles *c = malloc(numLines_c*sizeof(*c));
	int rtn = parseUpdate(c, projName, 2);
	if(rtn == -2) {
		fprintf(stderr, "Error opening .Update file.\n");
		free(c);
		free(buffer);
		free(pName);
		return;
	}

		// Gets the number of 'A' codes from the .commit file for adding to the malloc for .Manifest
	i = 0;
	int addM = 0;
	while(i < numLines_c) {
		if(strcmp(c[i].code, "A") == 0) {
			++addM;	
		}
		++i;
	}

		// Parse the .Manifest
	char mPath[strlen(projectPath) + 11];
	strcpy(mPath, projectPath);
	strcat(mPath, "/.Manifest");

	fd = open(mPath, O_RDONLY);
	
	cp = lseek(fd, (size_t)0, SEEK_CUR);	
	fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);
	
	char mBuf[(int)fileSize + 1];
	bzero(mBuf, (int)fileSize + 1);

	rfd = read(fd, mBuf, (int)fileSize);
	close(fd);
	mBuf[(int)fileSize] = '\0';
	
	i = 0;
	int numLines_m = 0;
	while(i < (int)fileSize) {
		if(mBuf[i] == '\n') {
			++numLines_m;
		}
		++i;
	}	
	
	files *m = malloc((numLines_m+addM)*(sizeof(*m)));
	parseManifest(m, pName);


	i = k = 0;
	int holder = numLines_m;
	int totalBytes = 0;
	while(i < numLines_c) {
		k = 0;
		while(k < numLines_m) {
			
				// Updating .Manifest with everything from .commit
			if(strcmp(c[i].file_name, m[k].file_name) == 0) {
				if(strcmp(c[i].code, "D") == 0) {
					m[k].version_number = 0;
					char toRemove[strlen(m[k].file_name) + 9];
					strcpy(toRemove, ".server/");
					strcat(toRemove, m[k].file_name);
					int rmv = remove(toRemove);
					totalBytes += (strlen(c[i].file_name) + strlen(c[i].file_hash));
				} else if(strcmp(c[i].code, "U") == 0) {
					bzero(m[k].file_hash, strlen(c[i].file_hash));
					strcpy(m[k].file_hash, c[i].file_hash);
					m[k].version_number = c[i].version_number;
					totalBytes += (strlen(c[i].file_name) + strlen(c[i].file_hash));
				}
				break;
			}
	
				// Inside commit and not Manifest ('A' code)
			if( (strcmp(c[i].file_name, m[k].file_name) != 0) && (k == numLines_m-1) ) {
				if(strcmp(c[i].code, "A") == 0) {
						// Add file into the .Manifest. Space already allocated
					strcpy(m[holder].file_name, c[i].file_name);
					strcpy(m[holder].file_hash, c[i].file_hash);
					m[holder].version_number = 1;
					totalBytes += (strlen(c[i].file_name) + strlen(c[i].file_hash));
					++holder;
				}
			}		
			++k;
		}
		++i;
	}

		// Increase manifest version number
	m[0].version_number += 1;


		// Rebuild the .Manifest
	rebuildManifest(m, mPath, (numLines_m + addM));


		// Update the .history with operations
	char hBuf[numLines_c*(totalBytes + 23)];
	bzero(hBuf, (numLines_c*(totalBytes + 23)));

	strcpy(hBuf, "push\n");
	char mVN[5];
	bzero(mVN, 5);
	sprintf(mVN, "%d", (m[0].version_number-1));
	strcat(hBuf, mVN);
	strcat(hBuf, "\n");
	i = 0;
	while(i < numLines_c) {
		char VN[5];
		bzero(VN, 5);
		sprintf(VN, "%d", c[i].version_number);
	
		strcat(hBuf, c[i].code);
		strcat(hBuf, " ");
		strcat(hBuf, VN);
		strcat(hBuf, " ");
		strcat(hBuf, c[i].file_name);
		strcat(hBuf, " ");
		strcat(hBuf, c[i].file_hash);
		strcat(hBuf, "\n");
		++i;
	}
	strcat(hBuf, "\n");

	fd = open(historyPath, O_APPEND | O_RDWR);
	if(fd < 0) {
		fprintf(stderr, "Error opening .history file on server.\n");
		free(c);
		free(m);
		free(buffer);
		free(pName);
		return;
	}

	write(fd, hBuf, strlen(hBuf));
	close(fd);

		// Remove .commit
	rmv = remove(c_commit);


		// Send server's .Manifest back to the client
	char sendM[strlen(mPath) + 34];
	bzero(sendM, (strlen(mPath) + 34));
	strcpy(sendM, "tar -czf .server/archive5.tar.gz ");
	strcat(sendM, mPath);
	

		// Tar/compress .Manifest and send it over to the client.
	sys = system(sendM);
	if(sys < 0) {
		fprintf(stderr, "Error compressing `.Manifest` in project: %s\n", pName);
		free(c);
		free(m);
		free(buffer);
		free(pName);
		return;
	}

	fd = open(".server/archive5.tar.gz", O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Error opening: .server/archive5.tar.gz\n");
		free(c);
		free(m);
		free(buffer);
		free(pName);
		return;
	}

	cp = lseek(fd, (size_t)0, SEEK_CUR);	
	fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);

	unsigned char sendMB[(int)fileSize + 1];

	int rd = read(fd, sendMB, (int)fileSize);
	if(rd < 0) {
		fprintf(stderr, "Error reading from: .server/archive5.tar.gz\n");	
		free(c);
		free(m);
		free(buffer);
		free(pName);
		return;
	}
	close(fd);

		// Send compressed project back to client.
	write(sockfd, sendMB, (int)fileSize);
	printf("Sent compressed .Manifest to client for evaluation.\n");
	

		// Remove compressed .Manifest
	rmv = remove(".server/archive5.tar.gz");

	free(c);
	free(m);
	free(buffer);
	free(pName);
	return;
}

void *history(void *input) {
	
	char* projName = ((struct args*)input)->str;
	int sockfd = ((struct args*)input)->num;

		// Will be set to 1 if projName exists
	int checkExist = 0;
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
						checkExist = 1;
						break;
					}
				}
			}
			status = readdir(d);
		} while(status != NULL);
		closedir(d);
	}

		// Project existence check
	if(checkExist == 0) {
		fprintf(stderr, "Error: Project does not exists on the server.\n");
		return;
	}

	
	char historyPath[strlen(projName) + 18];
	strcpy(historyPath, ".server/");
	strcat(historyPath, projName);
	strcat(historyPath, "/.history");

	
	int fd = open(historyPath, O_RDONLY);
	
	off_t cp = lseek(fd, (size_t)0, SEEK_CUR);	
	size_t fileSize = lseek(fd, (size_t)0, SEEK_END); 
	lseek(fd, cp, SEEK_SET);
	
	char hBuf[(int)fileSize + 1];
	bzero(hBuf, (int)fileSize + 1);

	int rfd = read(fd, hBuf, (int)fileSize);
	close(fd);
	hBuf[(int)fileSize] = '\0';


	write(sockfd, hBuf, strlen(hBuf));

	return;
}

int isfile(char *path) {
   struct stat statbuf;

   if (stat(path, &statbuf) == -1)
      return 0;
   else
      return S_ISREG(statbuf.st_mode);
}

void *rollback(void *input) {
	
	char* projName = ((struct args*)input)->str;
	int sockfd = ((struct args*)input)->num;
	
	int i = 0;
	while(i < strlen(projName)) {
		if(projName[i] == ':') {
			break;
		}
		++i;
	}

	char version[5];
	bzero(version, 5);
	memcpy(version, projName+i+1, strlen(projName));
	projName[i] = '\0';
	
	int versionNum = atoi(version);
	
		// Will be set to 1 if projName exists
	int checkExist = 0;
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
						checkExist = 1;
						break;
					}
				}
			}
			status = readdir(d);
		} while(status != NULL);
		closedir(d);
	}

		// Project existence check
	if(checkExist == 0) {
		fprintf(stderr, "Error: Project does not exists on the server.\n");
		return;
	}
	char projPath[strlen(projName) + 40];
	bzero(projPath, (strlen(projName) + 40));
	strcpy(projPath, ".server/");
	strcat(projPath, projName);
	
	d = opendir(projPath);
	if(d == NULL) {
		printf("directory does not exist\n");
		return;
	}
	
	//tar -xzf .server/test/.versions/1.tar.gz
	
	strcat(projPath, "/.versions/");
	strcat(projPath, version);
	strcat(projPath, ".tar.gz");
	//printf("projPath:%s\n", projPath);
	if(!isfile(projPath)){
		printf("tar does not exist\n");
		return;
	}
	
	char tarCommand[strlen(projPath)+11];
	bzero(tarCommand, (strlen(projPath) + 11));
	strcpy(tarCommand, "tar -xzf ");
	strcat(tarCommand, projPath);

		// Delete everything except the version and history folder/file
	rollbackDelete(projPath);

	int sys =  system(tarCommand);
	if(sys < 0) {
		fprintf(stderr, "Error on System() call.\n");
		return;
	}
	
		//delete all other version numbers
	char tarPath[strlen(projName) + 20];
	bzero(tarPath, (strlen(projName) + 20));
	strcpy(tarPath, ".server/");
	strcat(tarPath, projName);
	strcat(tarPath, "/.versions/");

	char tarFile[strlen(tarPath)+12];
	strcpy(tarFile, tarPath);
	
	char numberStr[3];
	sprintf (numberStr, "%d", ++versionNum);
	strcat(tarFile, numberStr);
	strcat(tarFile, ".tar.gz");
	
	while(isfile(tarFile)){
		int rmv = remove(tarFile);
		//
		bzero(tarFile, strlen(tarFile));
		
		strcpy(tarFile, tarPath);
		
		sprintf (numberStr, "%d", ++versionNum);
		
		strcat(tarFile, numberStr);
		strcat(tarFile, ".tar.gz");
	}
	write(sockfd, "Rolledback", 2);
	
	fprintf(stdout, "Successfully rolled back to version: %s.\n", version);

		// Append rollback to history.

	char historyPath[strlen(projName) + 18];
	strcpy(historyPath, ".server/");
	strcat(historyPath, projName);
	strcat(historyPath, "/.history");

	// rollback\n5\n\n
	char rMessage[20];
	bzero(rMessage, 20);

	strcpy(rMessage, "rollback\n");
	strcat(rMessage, version);
	strcat(rMessage, "\n\n");

	int fd = open(historyPath, O_APPEND | O_RDWR);
	if(fd < 0) {
		fprintf(stderr, "Error appending rollback message to .history. File not found.\n");
		return;
	}
	write(fd, rMessage, strlen(rMessage));
	close(fd);

	return;
}


int main(int argc, char *argv[] ) {

	if(argc != 2) {
		fprintf(stderr, "Invalid Number of Arguments.\nExpected a single Port #.\nReceived %d argument(s).\nUsage: ./WTFserver <PORT#>\n", argc - 1);
		exit(1);
	}

	int sockfd = -1;
	int newsockfd = -1;
	int clientAddrInfo = -1;
	// Server address/port struct
	struct sockaddr_in serverAddr;
	
	// Convert port string to int
	int port = atoi(argv[1]);
	if( port == 0 ) {
		fprintf(stderr, "Invalid Port.\n");
		exit(1);
	}	

		// Create server socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if( sockfd < 0 ) {
		perror("ERROR opening socket");
		exit(1);
	}

	

		// Initialize server Addr
	bzero((char*)&serverAddr, sizeof(serverAddr));	

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

		// Check return of binding
	if( bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0 ) {
		fprintf(stderr, "Error while binding server.\n");
		exit(1);
	}
	
	int command = 0;
	int n, i, j;
	i = n = j = 0;
	struct args *Params = (struct args *)malloc(sizeof(struct args));
	printf("Listening for connections..\n");
	//////////////////////////////////////////////////////////////////
	while(1) {
			// Listen for client connections
		listen(sockfd, 25);
		
			// Struct for client
		struct sockaddr_in clientAddr;
		clientAddrInfo = sizeof(clientAddr);
		//client connection
		//newsockfd is client name, make a data structure for all clients
		//need to loop somehow for multi threading
		newsockfd = accept(sockfd, (struct sockaddr*)&clientAddr, &clientAddrInfo);
		if(newsockfd >= 0) {
			printf("Successfully connected to a client.\n");
		}

		// pthread create - data, socket, global mutex lock for that structure

		fd_set set;
		struct timeval timeout;

		FD_ZERO(&set);
		FD_SET(newsockfd, &set);

		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		//ensures something is bring read from client
		select(FD_SETSIZE, &set, NULL, NULL, &timeout);
		//n is length of buffer
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
		
			// Get the command given from client. (Create, history, rollback, etc...)
			// 1 - create
			// 2 - destroy
			// 3 - checkout
			// 4 - currentversion
			// 5 - update
			// 6 - upgrade
			// 7 - commit
			// 8 - push
			// 9 - history
			// 10 - rollback
			
		command = getCommand(buffer);
		Params->str = getProjectName(buffer);
		Params->num = newsockfd;
		
		if(command == 1) {
			
			pthread_t tid0;
			pthread_create(&tid0, NULL, create, (void *)Params);
			pthread_join(tid0, NULL);
			
		} else if(command == 2) {
			
			pthread_t tid0;
			pthread_create(&tid0, NULL, destroy, (void *)Params);
			pthread_join(tid0, NULL);
			
		} else if(command == 3) {
			
			pthread_t tid0;
			pthread_create(&tid0, NULL, checkout, (void *)Params);
			pthread_join(tid0, NULL);
			
		} else if(command == 4) {
			
			pthread_t tid0;
			pthread_create(&tid0, NULL, currentversion, (void *)Params);
			pthread_join(tid0, NULL);

		} else if(command == 5) {
			
			pthread_t tid0;
			pthread_create(&tid0, NULL, update, (void *)Params);
			pthread_join(tid0, NULL);
			
		} else if(command == 6) {
			
			pthread_t tid0;
			pthread_create(&tid0, NULL, upgrade, (void *)Params);
			pthread_join(tid0, NULL);
			
		} else if(command == 7) {
			
			pthread_t tid0;
			pthread_create(&tid0, NULL, commit, (void *)Params);
			pthread_join(tid0, NULL);
			
		} else if(command == 8) {
			
			pthread_t tid0;
			pthread_create(&tid0, NULL, push, (void *)Params);
			pthread_join(tid0, NULL);
			
		} else if(command == 9) {
			
			pthread_t tid0;
			pthread_create(&tid0, NULL, history, (void *)Params);
			pthread_join(tid0, NULL);
			
		} else if(command == 10) {
			
			pthread_t tid0;
			pthread_create(&tid0, NULL, rollback, (void *)Params);
			pthread_join(tid0, NULL);
			
		} else {
		
		}
		//sleep(1);
	}
	//////////////////////////////////////////////////////////////////
	close(newsockfd);
	close(sockfd);
	free(Params->str);
	return 0;
}
