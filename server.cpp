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
#include <pthread.h>
#include <sstream>

using namespace std;

struct player {
    int score;
    char name[1024];
};

player leaderboard[3];

//ensures that only one file is open at a time
pthread_mutex_t myMutex;

void* SocketHandler(void*);

int main(int argc, char *argv[]){

    if(argc != 2){
	  printf("Valid port number required (err: %d)\n", errno);
	  exit(-1);
	}
	
    int host_port = atoi(argv[1]);
	struct sockaddr_in my_addr;
	
	int hsock;
	int * p_int ;
	int err;
	
	pthread_mutex_init(&myMutex,0);
	socklen_t addr_size = 0;
	int* csock;
	sockaddr_in sadr;
	pthread_t thread_id=0;
	
	//Creating a TCP socket
	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1){
	  printf("Error initializing socket %d\n", errno);
	  exit(-1);
	}
	
	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;
	
	//Sets options for the socket
	if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
		(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
	  printf("Error setting options %d\n", errno);
	  free(p_int);
	  exit(-1);
	}
	free(p_int);
	
	//Sets the port
	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);
	
	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = INADDR_ANY ;
	
	//Binds the port to the socket
	if( bind( hsock, (sockaddr*)&my_addr, sizeof(my_addr)) == -1 ){
	  fprintf(stderr,"Error binding to socket, make sure nothing else is listening on this port %d\n",errno);
	  exit(-1);
	}
	
	//Sets the socket to listen
	if(listen( hsock, 10) == -1 ){
	  fprintf(stderr, "Error listening %d\n",errno);
	  exit(-1);
	}
	
	addr_size = sizeof(sockaddr_in);
	printf("waiting for a connection...\n");
	
	while(true){
	  csock = (int*)malloc(sizeof(int));
	  if((*csock = accept( hsock, (sockaddr*)&sadr, &addr_size))!= -1){
		printf("---------------------\nReceived connection from %s\n",inet_ntoa(sadr.sin_addr));
		pthread_create(&thread_id,0,&SocketHandler, (void*)csock );
		pthread_detach(thread_id);
	  } else {
		fprintf(stderr, "Error accepting %d\n", errno);
	  }
	}
}

void* SocketHandler(void* lp){
    int *csock = (int*)lp;
	char buffer[1024];	
	int buffer_len = 1024;
	stringstream ss;
	int bytecount;
	int playerGuess;
	int sum;
	srand(time(NULL));
	int target = rand() % 10000;
	printf("Random Number: %d\n", target);
  
	memset(buffer, '\0' , buffer_len);
	if((bytecount = recv(*csock, buffer, buffer_len, 0))== -1){
	    fprintf(stderr, "Error receiving data %d\n", errno);
		abort();
	}
	
	player p;
	p.score = 0;
	strcpy(p.name, buffer);

	//plays the game with the client
	do {
	    memset(buffer, '\0' , buffer_len);
		if((bytecount = recv(*csock, buffer, buffer_len, 0))== -1){
		    fprintf(stderr, "Error receiving data %d\n", errno);
			break;
		}
		p.score++;
		int temp = target;
		sum = 0;
		playerGuess = atoi(buffer);
		memset(buffer, '\0' , buffer_len);
		ss.str("");
		for (int i = 0; i < 4; i++){
		    sum = sum + abs((playerGuess % 10) - (temp % 10));
			playerGuess = playerGuess/10;
			temp = temp/10;
		}
		ss << sum;
		strcpy(buffer, ss.str().c_str());
		if((bytecount = send(*csock, buffer, strlen(buffer), 0))== -1){
		    fprintf(stderr, "Error sending data %d\n", errno);
		    break;
		}
	} while (sum != 0);
	
	if((bytecount = recv(*csock, buffer, buffer_len, 0))== -1){
	  fprintf(stderr, "Error receiving data %d\n", errno);
	}
	
	//only one client can edit and read the leaderboard at a time
	pthread_mutex_lock(&myMutex);
	
	int turns = p.score;
	//matches current player score against leaderboard scores
	for (int i = 0; i < 3; i++){ 
	    if (leaderboard[i].score == 0){
		    leaderboard[i] = p;
		    break;
		} else if (leaderboard[i].score > p.score) {
		    player n = leaderboard[i];
		    leaderboard[i] = p;
		    p = n;
		}
	}
	
	//creates the victory message and sends it to the player
	ss.str("");
	ss << "Congratulations! It took " << turns;
	ss << " turns to guess the number!\n\n";
	//leaderboard appears only after at least 3 players have scores
	if (leaderboard[2].score != 0){ 
	    ss << "Leader board:\n";
		ss << "1. " << leaderboard[0].name << " " << leaderboard[0].score << "\n";
		ss << "2. " << leaderboard[1].name << " " << leaderboard[1].score << "\n";
		ss << "3. " << leaderboard[2].name << " " << leaderboard[2].score << "\n";
	} else {
	  if (leaderboard[1].score == 0)
		ss << "Current Entries: 1\n";
	  else
		ss << "Current Entries: 2\n";
	}
	memset(buffer, '\0' , buffer_len);
	strcpy(buffer, ss.str().c_str());
	
	if((bytecount = send(*csock, buffer, strlen(buffer), 0))== -1){
        fprintf(stderr, "Error sending data %d\n", errno);
	}

	pthread_mutex_unlock(&myMutex);
	
	free(csock);
    return 0;
}
