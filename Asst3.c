#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#define BACKLOG 5

//recieves the messages and makes sure they follow protocol
char* protocol(char* buffer, char* expectation, int num) {

	//printf("Given: %s cmp: %d  num: %d\n", buffer, strncmp(buffer, "REG", 3), num);

	//make sure its a REG, if not wrong format
	if( strncmp(buffer, "REG|", 4) != 0 ) {
		
		if(num == 1) {
			return "ERR|M1FT|";
		}

		if(num == 3) {
			return "ERR|M3FT|";
		}

		if(num == 5) {
			return "ERR|M5FT|";
		}

	}

	//now check the length 
	int len = 0; 
	int i = 4;
	for(i = 4; i < strlen(buffer); i++) {


		//stop when finding the next |
		if(buffer[i] == '|') {
			
			//if len was never changed then a size was not provided
			if(len == 0) {
				if(num == 1) {
					return "ERR|M1LN|";
				}

				if(num == 3) {
					return "ERR|M3LN|";
				}

				if(num == 5) {
					return "ERR|M5LN|";
				}
			}

			break;
		}

		//if its a number add it to the length
		if( isdigit(buffer[i])) {
			len*= 10;
			len += buffer[i] - '0';
		}
		//not a number before finding the next | therefore an error.
		else{
			if(num == 1) {
				return "ERR|M1LN|";
			}

			if(num == 3) {
				return "ERR|M3LN|";
			}

			else {
				return "ERR|M5LN|";
			}
		}

	}

	//printf("Len given: %d   strlen of buffer: %ld\n", len, strlen(buffer));
	//printf("Total: %d\n", len+(i)+2);

	//make sure the length matches the size of what was given
	if( strlen(buffer) != len+i+2 ) {
		if(num == 1) {
			return "ERR|M1LN|";
		}

		if(num == 3) {
			return "ERR|M3LN|";
		}

		else {
			return "ERR|M5LN|";
		}
	}
	
	//make sure the text recieved matches what is expected.
	char* msg = malloc(len+1);
	memcpy(msg, &buffer[i+1], len);
	//printf("Msg: '%s'\n", msg);	
	//printf("Ex: '%s'\n", expectation);

	//the 5th message can be anything, get passed as NULL.
	if(num == 5) {

		//check that there is punctuation. 
		if(buffer[ strlen(buffer) - 2 ] ==  '.' || buffer[ strlen(buffer) - 2 ] ==  '!' || buffer[ strlen(buffer) - 2 ] ==  '?') {
			//ended with the possible punctuation. 
			return "Success";
			
		}

		//there was no the required punctuation.  

		return "ERR|M5CT|";
	}

	//make sure the buffer contains the text that was expected.
	if( strcmp( msg, expectation ) != 0 ) {
		if(num == 1) {
			return "ERR|M1CT|";
		}

		if(num == 3) {
			return "ERR|M3CT|";
		}

		else {
			return "ERR|M5CT|";
		}
	}

	//there were no errors.

	return "Success";
}


int main(int argc, char** argv) {

	//check for the right number for arguments
	if(argc != 2) {
		perror("Invalid Port Input");
		exit(EXIT_FAILURE);
	}

	//make sure port number is within range
	int port = atoi(argv[1]); 
	if(port <= 5000 || port >= 65536) {
		perror("Invalid Port Range");
		exit(EXIT_FAILURE);
	}

	printf("Port Recieved: %s\n", argv[1]);

	//will be passed to getaddrinfo function
	struct addrinfo hint; 
	//will be a lisy modified as a result of getaddrinfo
	struct addrinfo *address_res; 
	//will hold the traverses values from the list. 
	struct addrinfo *addr; 	
	
		
	//make sure hint is empty
    	memset(&hint, 0, sizeof(struct addrinfo));
	//initialize it 
   	hint.ai_family = AF_UNSPEC;	//IPv4 or IPv6
    	hint.ai_socktype = SOCK_STREAM;	
    	hint.ai_flags = AI_PASSIVE;	//listening socket

	int error = getaddrinfo(NULL, argv[1], &hint, &address_res); 
	//check for error
	if(error != 0) {
		perror("Could not get address");
		return -1;  
	}
	//address function worked

	//will hold the socket/file descriptor.
	int sfd; 

	for(addr = address_res; addr != NULL; addr = addr->ai_next) {

		//call the socket function with the values obtain from getaddrinfo
		sfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

		//test for error
		if(sfd == -1) {
			//move on to the next in the list
			continue;
		}	

		//the socket descriptor is obtained. Now bind it
		int retBind = bind(sfd, addr->ai_addr, addr->ai_addrlen); 
		if(retBind == -1) {
			close(sfd);
			continue;
		}

		//now that it is binded to a port, now listen for incoming input to later accept. 
		int retListen = listen(sfd, BACKLOG); //BACKLOG is the number of inputs allowed which is defined. 
		if(retListen == -1) {
			close(sfd);
			continue;
		}

		//Now that it has been succesfully binded and listened break to use this value of addr
		break;	
	
	}
	
	//Check to make sure the loop broke after it was binded and listened, and not because it was unsuccesful
	if(addr == NULL) {
		perror("Could not bind and listen");
		close(sfd);
		return -1;
	} 

	//free the address as it is no longer in use.
	freeaddrinfo(address_res); 

	
	//now that it is listening, accept the incoming input
	printf("Listening...\n");

	//create the accept parameters
	struct sockaddr_storage sock_res; //accept write to 
	socklen_t addr_len = sizeof(struct sockaddr_storage); //will always be a length less or equal to this size
	int fd;	//new file descriptor

	while(1) { 

		//printf("Waiting to recieve something...\n");

		//accept it		
		fd = accept(sfd, (struct sockaddr*) &sock_res, &addr_len );
		
		//make sure it worked
		if(fd == -1) {
			perror("Could not Accept");
			close(fd);
			close(sfd);
			return -1; 
		}

		//Now that something has correctly accepted, Write Knock Knock to them (W1) 
				
		char* knock = "REG|13|Knock, knock.|"; 
		
		int resW = write(fd, knock, strlen(knock));
		if(resW == -1) {
			perror("Could not respond");
			close(fd);
			close(sfd);
			return -1; 
		}
		printf("	Sent: '%s'\n", knock);
		
		
		//make the buffer 1 greater to account for '\0', since read is looped the size does not matter. 
		char *buffer1 = malloc(100);
		char c; 
		int size = 100; 
		int bracketCount = 0;
		int bufferIndex = 0;
		while(1) {

			int readRet = read(fd, &c, 1); 
			if(readRet == -1) {
				perror("Could not respond");
				close(fd);
				close(sfd);
				return -1; 
			}

			//printf("c: %c\n", c);
			buffer1[bufferIndex] = c;
			buffer1[bufferIndex+1] = '\0';
			bufferIndex++;	
	
			//printf("Buffer 1: '%s'\n", buffer1);

			//check to make sure the entire message was recieved, read until end of line, *there is no testcase that the last "|" will be missing otherwise read would never break out of loop.  
			for(int i = 0; i < size; i++) {
				if(buffer1[i] == '\0') {		
					break; 
				}	

				if(buffer1[i] == '|') {
					bracketCount++; 
				}			
			}

			//if there are less than 2 "|" or more than 3 then immediately a formating error. 
			if(bracketCount > 3) {
				perror("Error: M1FT");
				//bad protocol, write to client. Not checking return value since shutting down either way. 
				write(fd, "ERR|M1FT|", 9);

				close(fd);
				close(sfd);
				return -1;
						 
			}

			//if there are 3 then it is done reading, call protocol function
			if(bracketCount == 3) {
			 
				printf(" (1) Recieved: '%s'\n ", buffer1);	
		
				//check the return value of the protocol, if it returns as successful then the message follows protocol. Otherwise returns error code. 
				char* retP = protocol(buffer1, "Who's there?", 1);
			
				if(strcmp(retP, "Success") != 0) {
					printf("Error: %s\n", retP);
					
					//bad protocol, write to client. Not checking return value since shutting down either way. 
					write(fd, retP, 9);
					
					close(fd);
					close(sfd);	
					return -1; 		
				}	

				break; 	
			}

			//if there are 2 or less then it needs to read more or it is done reading an error message
			else if(bracketCount <= 2) {
				
				//Report the error type for m1. 
				if( strncmp(buffer1, "ERR", 3) == 0 ) {
					
					if(strcmp(buffer1, "ERR|M1CT") == 0) {
						perror("Error: M1CT");

						//bad protocol, write to client. Not checking return value since shutting down either way. 
						write(fd, "ERR|M1CT|", 9);
				
						close(fd);
						close(sfd);
						return -1; 
					}
					
					if(strcmp(buffer1, "ERR|M1LN") == 0) {
						perror("Error: M1LN");

						//bad protocol, write to client. Not checking return value since shutting down either way. 
						write(fd, "ERR|M1LN|", 9);				

						close(fd);
						close(sfd);
						return -1; 
					}


					else {
						perror("Error: M1FT");

						//bad protocol, write to client. Not checking return value since shutting down either way. 
						write(fd, "ERR|M1FT|", 9);

						close(fd);
						close(sfd);
						return -1; 
					}
					
				}

				//not done reading, reallocate in case it needs more room for the next read. 
				else{
					bufferIndex = strlen(buffer1);
					//if it needs more room for the next read. 
					if(strlen(buffer1) + 10 >= size) {

						size *= 2; 
						buffer1 = realloc(buffer1, size);	
						
					}
					bracketCount = 0;

				}

			}
			
		}	
		
		//free the buffer;
		free(buffer1);

		
		
		//respond with the set up line (W2)
		char* setup = "Orange."; 
		
		int resW2 = write(fd, setup, strlen(setup));
		if(resW2 == -1) {
			perror("Could not respond");
			close(fd);
			close(sfd);
			return -1; 
		}
		printf("	Sent: '%s'\n", setup);	
		
		//read the response the the set up. 
		char *buffer2 = malloc(100);
		char c2;  
		int size2 = 100;
		int bracketCount2 = 0;
		int bufferIndex2 = 0;
		while(1) {

			int readRet2 = read(fd, &c2, 1); 
			if(readRet2 == -1) {
				perror("Could not respond");
				close(fd);
				close(sfd);
				return -1; 
			}
			//printf("c2: %c\n", c2);
			buffer2[bufferIndex2] = c2;
			buffer2[bufferIndex2+1] = '\0';
			bufferIndex2++;		

			//printf("Buffer 2: '%s'\n", buffer2);

			//check to make sure the entire message was recieved, read until end of line, *there is no testcase that the last "|" will be missing otherwise read would never break out of loop.  
			for(int i = 0; i < size2; i++) {
				if(buffer2[i] == '\0') {		
					break; 
				}	

				if(buffer2[i] == '|') {
					bracketCount2++; 
				}			
			}


			//if there are less than 2 "|" or more than 3 then immediately a formating error. 
			if(bracketCount2 > 3) {
				perror("Error: M3FT");
	
				//bad protocol, write to client. Not checking return value since shutting down either way. 
				write(fd, "ERR|M3FT|", 9);

				close(fd);
				close(sfd);
				return -1; 
						 
			}

			//if there are 3 then it is done reading, call protocol function
			if(bracketCount2 == 3) {
			 
				printf(" (2) Recieved: '%s'\n ", buffer2);	
		
				//check the return value of the protocol, if it returns as REG then the message follows protocol. Otherwise returns error code. 
				char* retP = protocol(buffer2, "Orange, who?", 3);
			
				if(strcmp(retP, "Success") != 0) {
					printf("Error: %s\n", retP);

					//bad protocol, write to client. Not checking return value since shutting down either way. 
					write(fd, retP, 9);

					close(fd);
					close(sfd);	
					return -1; 		
				}	

				break; 	
			}

			//if there are 2 or less then it needs to read more or it is done reading an error message
			else if(bracketCount2 <= 2) {
				
				//Report the error type for m3. 
				if( strncmp(buffer2, "ERR", 3) == 0 ) {
					
					if(strcmp(buffer2, "ERR|M3CT") == 0) {
						perror("Error: M3CT");

						//bad protocol, write to client. Not checking return value since shutting down either way. 
						write(fd, "ERR|M3CT|", 9);

						close(fd);
						close(sfd);
						return -1; 
					}
					
					if(strcmp(buffer2, "ERR|M3LN") == 0) {
						perror("Error: M3LN");

						//bad protocol, write to client. Not checking return value since shutting down either way. 
						write(fd, "ERR|M3LN|", 9);

						close(fd);
						close(sfd);
						return -1; 
					}


					else {
						perror("Error: M3FT");

						//bad protocol, write to client. Not checking return value since shutting down either way. 
						write(fd, "ERR|M3FT|", 9);

						close(fd);
						close(sfd);
						return -1; 
					}
					
				}

				//not done reading, reallocate in case it needs more room for the next read. 
				else{
			

					bufferIndex2 = strlen(buffer2);
					//if it needs more room for the next read. 
					if(strlen(buffer2) + 10 >= size2) {

						printf("Realloc\n");
						size2 *= 2; 
						buffer2 = realloc(buffer2, size2);	
						
					}
					bracketCount2 = 0;

				}

			}
			
		}
		
		//free
		free(buffer2);
	
		

		//send the punchline
		char* punchline = "Orange you glad that I didn't say banana."; 
		
		int resW3 = write(fd, punchline, strlen(punchline));
		if(resW3 == -1) {
			perror("Could not respond");
			close(fd);
			close(sfd);
			return -1; 
		}
		printf("	Sent: '%s'\n", punchline);	
			
		//read their reaction to the punchline
		char *buffer3 = malloc(100);
		char c3; 
		int size3 = 100; 
		int bracketCount3 = 0;
		int bufferIndex3 = 0;
		

		while(1) {
			
			int read3 = read(fd, &c3, 1);
			if(read3 == -1) {
				perror("Could not read\n");
				close(fd);
				close(sfd);
				return -1;
 
			} 
			//printf("c3: %c\n", c3);
			buffer3[bufferIndex3] = c3;
			buffer3[bufferIndex3+1] = '\0';
			bufferIndex3++;	
			
			//printf("Buffer 3: '%s'\n", buffer3);

			

			//check to make sure the entire message was recieved, read until end of line, *there is no testcase that the last "|" will be missing otherwise read would never break out of loop.  
			for(int i = 0; i < size3; i++) {
				if(buffer3[i] == '\0') {		
					break; 
				}	

				if(buffer3[i] == '|') {
					bracketCount3++; 
				}			
			}

			//if there are less than 2 "|" or more than 3 then immediately a formating error. 
			if(bracketCount3 > 3) {
				perror("Error: M5FT");

				//bad protocol, write to client. Not checking return value since shutting down either way. 
				write(fd, "ERR|M5FT|", 9);

				close(fd);
				close(sfd);
				return -1; 
						 
			}

			//if there are 3 then it is done reading, call protocol function
			if(bracketCount3 == 3) {
			 
				printf(" (3) Recieved: '%s'\n ", buffer3);	
		
				//check the return value of the protocol, if it returns as REG then the message follows protocol. Otherwise returns error code. 
				char* retP = protocol(buffer3, NULL, 5);

				if(strcmp(retP, "Success") != 0) {
					printf("Error: %s\n", retP);

					//bad protocol, write to client. Not checking return value since shutting down either way. 
					write(fd, retP, 9);

					close(fd);
					close(sfd);
					return -1; 			
				}	

				//printf("PROTO3: %s\n", retP);

				break; 	
			}

			//if there are 2 or less then it needs to read more or it is done reading an error message
			else if(bracketCount3 <= 2) {
				
				//Report the error type for m3. 
				if( strncmp(buffer3, "ERR", 3) == 0 ) {
					
					if(strcmp(buffer3, "ERR|M5CT") == 0) {
						perror("Error: M5CT");

						//bad protocol, write to client. Not checking return value since shutting down either way. 
						write(fd, "ERR|M5CT|", 9);

						close(fd);
						close(sfd);
						return -1; 
					}
					
					if(strcmp(buffer3, "ERR|M5LN") == 0) {
						perror("Error: M5LN");

						//bad protocol, write to client. Not checking return value since shutting down either way. 
						write(fd, "ERR|M5LN|", 9);

						close(fd);
						close(sfd);
						return -1; 
					}


					else {
						perror("Error: M5FT");

						//bad protocol, write to client. Not checking return value since shutting down either way. 
						write(fd, "ERR|M5FT|", 9);

						close(fd);
						close(sfd);
						return -1; 
					}
					
				}

				//not done reading, reallocate in case it needs more room for the next read. 
				else{
			

					bufferIndex3 = strlen(buffer3);
					//if it needs more room for the next read. 
					if(strlen(buffer3) + 10 >= size3) {

						size3 *= 2; 
						buffer3 = realloc(buffer3, size3);	
						
					}
				
					bracketCount3 = 0;

				}

			}
			
		}
		
		//free
		free(buffer3);

		//close
		close(fd);
		
		//divider
		printf("\nListening...\n");
		
	}

	close(fd); 

	
	return 0; 
}




