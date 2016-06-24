#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>

int main(int argc, char* argv[]){

    if (argc != 3){
	  printf("Valid IP address and port number required (err: %d)\n", errno);
	  exit(-1);
	}
	
    int host_port = atoi(argv[2]);
	char* host_name = argv[1];
	  
	struct sockaddr_in my_addr;

	char buffer[1024];
	int bytecount;
	int buffer_len=0;

	int hsock;
	int * p_int;
	int err;

	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1){
		printf("Error initializing socket %d\n",errno);
		abort();
	}

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);

	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = inet_addr(host_name);

	if( connect( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1 ){
		if((err = errno) != EINPROGRESS){
 			fprintf(stderr, "Error connecting socket %d\n", errno);
			abort();
		}
	}

	buffer_len = 1024;
	printf("Welcome to Number Guessing Game!\n");
	printf("Enter your name: ");
	fgets(buffer, 1024, stdin);
	printf("\n");
	buffer[strlen(buffer)-1]='\0';
	int count = 0;
	//Sends the player name to the server
	if( (bytecount=send(hsock, buffer, strlen(buffer),0))== -1){
	  fprintf(stderr, "Error sending data %d\n", errno);
	  abort();
	}
	do {
	  count++;
	  printf("Turn: %d\n", count);
	  bool valid;
	  
	  do {
		memset(buffer, '\0', buffer_len);
		printf("Enter a guess: ");
		fgets(buffer, 1024, stdin);
		valid = true;
		if (strlen(buffer) > 5){
		  valid = false;
		  printf("Input cannot exceed four digits.\n");
		}
		for (int i = 0; i < strlen(buffer) - 1; i++){
		  if(!isdigit(buffer[i])){
			valid = false;
			printf("Input must be a positive integer.\n");
			break;
		  }
		}
		if (strcmp(buffer, "\n") == 0) {
		  valid = false;
		  printf("Please enter a valid integer.\n");
		}
	  } while(!valid);
  
	  buffer[strlen(buffer)-1]='\0';

	  if( (bytecount=send(hsock, buffer, strlen(buffer),0))== -1){
		fprintf(stderr, "Error sending data %d\n", errno);
		break;
	  }
	  memset(buffer, '\0', buffer_len);
	  if((bytecount = recv(hsock, buffer, buffer_len, 0))== -1){
		fprintf(stderr, "Error receiving data %d\n", errno);
	    break;
	  }
 
	  printf("Result of guess: ");
	  printf("%s\n\n", buffer);
	 	  
	} while (strcmp(buffer, "0") != 0);

	//signals that player is ready to end game
	if( (bytecount=send(hsock, buffer, strlen(buffer),0))== -1){
	  fprintf(stderr, "Error sending data %d\n", errno);
	}	  
	  
	
	if((bytecount = recv(hsock, buffer, buffer_len, 0))== -1){
	  fprintf(stderr, "Error receiving data %d\n", errno);
  	}	
	printf("%s", buffer);
	close(hsock);
}
