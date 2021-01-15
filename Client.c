#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <limits.h>
#include <arpa/inet.h>
#include <sys/time.h>



#define MAX_IP 45

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

int check(int exp,const char *msg);
void getInput(int argc, char *argv[],char* host,int* port,int* src,int* dest);
void displayUsage();
unsigned long t();

int main(int argc, char **argv){
    struct timeval stop, start;
    int server_socket, port,src,dest;
    SA_IN server_addr;
    char host[MAX_IP];

    getInput(argc,argv,host,&port,&src,&dest);

    check((server_socket = socket(AF_INET,SOCK_STREAM,0)),"Failed to create socket");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    check(inet_pton(AF_INET,host,&server_addr.sin_addr),"Invalid address\n");
    printf("%lu Client (%d) connecting to %s:%d\n",t(),getpid(),host,port);
    check(connect(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr)),"Connection Error");

    printf("%lu Client (%d) connected and requesting a path from node %d to %d\n",t(),getpid(),src,dest);
    send(server_socket,&src,sizeof(int),0);
    send(server_socket,&dest,sizeof(int),0);
 
    int byte;
    gettimeofday(&start, NULL);
    recv(server_socket,&byte,sizeof(int),0);
    if(byte == -1){
        gettimeofday(&stop, NULL);
        float diff = ((stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec)/1000000.0;
        printf("%lu Server’s response (%d): NO PATH, arrived in %.3f seconds, shutting down\n",t(),getpid(),diff);
    }
    else{
        char* res = (char*) malloc(sizeof(char)*(byte+1));
        recv(server_socket,res,byte,0);
        gettimeofday(&stop, NULL);
        res[byte] = '\0';
        float diff = ((stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec)/1000000.0;
        printf("%lu Server’s response to (%d): %s, arrived in %.3f seconds.\n",t(),getpid(),res,diff);
        free(res);
    }


    close(server_socket);

    

    return 0;
}

int check(int exp,const char *msg){
    if(exp == -1){
        perror(msg);
        exit(1);
    }
    return exp;
}

void getInput(int argc, char *argv[],char* host,int* port,int* src,int* dest){
    	int opt;

    if(argc != 9){
        displayUsage();
        exit(1);
    }

	while( (opt = getopt(argc,argv,"a:p:s:d:")) != -1 )
	{
		switch(opt)
		{
			case 'a':
				strcpy(host,optarg);
				break;
			case 'p':
                *port = atoi(optarg);
				break;
			case 's':
				*src = atoi(optarg);
				break;
			case 'd':
				*dest = atoi(optarg);
				break;
			default:
				displayUsage();
				exit(0); //TO-DO

		}
	}
}

void displayUsage(){
    printf("Usage: ./client -a 127.0.0.1 -p PORT -s 768 -d 979\n");
}

unsigned long t(){
    return (unsigned long)time(NULL);
}
