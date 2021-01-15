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
#include <sys/time.h>
#include <time.h>
#include "Graph.h"

typedef struct CNode{
    char* path;
    int d;
    struct CNode* next;
}CNode;

CNode* database;

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

pthread_t* thread_pool;
pthread_t check_pool;
int clientSock, busyThread = 0;
bool client_f = false;
Graph* graph;

pthread_mutex_t mutex_db = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutex_r = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_r = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_main = PTHREAD_COND_INITIALIZER;
//int* pool_status;
int pool_size;
int busy_num = 0;
int idle_num = 0;
int max_thrd;
int flagh = false;
int logfd;

void* handle_connection(int client_socket, int num);
void *thread_function(void *arg);
void* resizer(void *arg);
void displayUsage();
void getInput(int argc, char *argv[],char* path,int* port,char* logfile,int* startup_t,int* max_t);
void loadGraph(Graph* g,int fd);
unsigned long t();
void freeDatabase();


void handler(int sig){
    if(sig == SIGINT){
        char buf[128];
        sprintf(buf,"%lu Termination signal received, waiting for ongoing threads to complete.\n",t());
        while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
        flagh = true;
    }
        
}

void cleanup(){

    freeGraph(graph);
    freeDatabase(database);
    pthread_cond_broadcast(&cond);
    pthread_cond_broadcast(&cond_r);
    pthread_cond_broadcast(&cond_main);
    for (int i = 0; i < pool_size; i++)
    {
        pthread_join(thread_pool[i], NULL);
    }
    free(thread_pool);
    pthread_join(check_pool, NULL);
    char buf[128];
    sprintf(buf,"%lu All threads have terminated, server shutting down.\n",t());
    while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
}


int main(int argc, char **argv){
	sigset_t nset;
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    sigaction(SIGINT, &act, 0);
	sigemptyset(&nset);
	sigaddset(&nset, SIGINT);
    struct timeval stop, start;
    float diff;
    
    graph = graph_create();
    //Input Variables
    char path[PATH_MAX];
    char logfile[PATH_MAX];
    int port, startup_t, max_t;
    getInput(argc,argv,path,&port,logfile,&startup_t,&max_t);
     char buf[1024];
    if(startup_t < 2){
        sprintf(buf,"%lu s should be bigger than 1\n",t());
        while(write(STDOUT_FILENO,buf,strlen(buf)) == -1 && (errno == EINTR));
        displayUsage();
        freeGraph(graph);
        exit(1);
    }

    if(max_t < startup_t){
        sprintf(buf,"%lu x can not less than s\n",t());
        while(write(STDOUT_FILENO,buf,strlen(buf)) == -1 && (errno == EINTR));
        displayUsage();
        freeGraph(graph);
        exit(1);        
    }
    logfd = open(logfile, O_CREAT | O_TRUNC,0666);
    if(logfd == -1){
        perror("Log: ");
        exit(1);
    }   

    close(logfd);
    int fd = open(path,O_RDONLY);
    if(fd == -1){
        sprintf(buf,"%lu Cannot Open files\n",t());
        while(write(STDOUT_FILENO,buf,strlen(buf)) == -1 && (errno == EINTR));
        displayUsage();
        freeGraph(graph);
        exit(1);
    }



    pid_t pid = fork();
    if (pid == -1) {
        sprintf(buf,"%lu Fork error\n",t());
        while(write(STDOUT_FILENO,buf,strlen(buf)) == -1 && (errno == EINTR));
        exit(1);
    } else if (pid != 0) {
        exit(0);
    }

    umask(0);

    logfd = open(logfile,O_WRONLY | O_APPEND,0666);
    if(logfd == -1){
        perror("Log: ");
        exit(1);
    }      

    pid_t sid = setsid();
    if (sid < 0) {
            /* Log any failure */
            exit(0);
    }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    sprintf(buf,"%lu Executing with parameters:\n",t());
    while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
    sprintf(buf,"%lu -i %s\n",t(),path);
    while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
    sprintf(buf,"%lu -p %d\n",t(),port);
    while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
    sprintf(buf,"%lu -o %s\n",t(),logfile);
    while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
    sprintf(buf,"%lu -s %d\n",t(),startup_t);
    while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
    sprintf(buf,"%lu -x %d\n",t(),max_t);
    while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));

    sprintf(buf,"%lu Loading graph...\n",t());
    while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
    gettimeofday(&start, NULL);
    loadGraph(graph,fd);
     database =  (CNode*) calloc(graph->indx,sizeof(CNode));;
    gettimeofday(&stop, NULL);
    diff = ((stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec)/1000000.0;

    sprintf(buf,"%lu Graph loaded in %.3f seconds with %d nodes and %d edges.\n",t(),diff,graph->indx,graph->edge);
    while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
    

    //Allocation for thread pool
    pool_size = startup_t;
    max_thrd = max_t;
    thread_pool = (pthread_t*) malloc(startup_t*sizeof(pthread_t));
    
    // Socket Variable
    int server_socket, client_socket, addr_size;
    SA_IN server_addr, client_addr;
    int r = pthread_create(&check_pool,NULL,&resizer,NULL);
    if(r != 0){
            sprintf(buf,"%lu Create Thread Error\n",t());
            while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
            exit(1);       
    }
    sprintf(buf,"%lu A pool of %d threads has been created\n",t(),startup_t);
    while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
    //Create threads
    for (int i = 0; i < startup_t; i++)
    {
        int s = pthread_create(&thread_pool[i],NULL,&thread_function,(void*) &i);
        if(s != 0){
            sprintf(buf,"%lu Create Thread Error\n",t());
            while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
            exit(1);
        }
    }
   
    idle_num = startup_t;
    
    // Get socket variable for server
    server_socket = socket(AF_INET,SOCK_STREAM,0);
    if(server_socket == -1){
        sprintf(buf,"%lu Create Socket Error\n",t());
        while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
        exit(1);        
    }

    // Initiliaze server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Socket bind to server address and listen port
    r = bind(server_socket,(SA*)&server_addr,sizeof(server_addr));
    if(r < 0){
        sprintf(buf,"%lu Bind Socket Error\n",t());
        while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
        exit(1);          
    }
    r = listen(server_socket,pool_size);
    if(r < 0){
         sprintf(buf,"%lu Listen Socket Error\n",t());
        while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
        exit(1);         
    }
    while(true){
        
        pthread_mutex_lock(&mutex_r);
        while(pool_size == busy_num){
            sprintf(buf,"%lu No thread is available! Waiting for one.\n",t());
            
            while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
            pthread_cond_wait(&cond_main,&mutex_r);
        }
         pthread_mutex_unlock(&mutex_r);

        if(flagh){
            cleanup();
            close(server_socket);
            return 0;
        }

        addr_size = sizeof(SA_IN);
        // Wait for connection
        client_socket = accept(server_socket,(SA*)&client_addr,(socklen_t*)&addr_size);
        


        if(client_socket != -1){
            // Client socket variable assign to global and lock,signal,unlock
            pthread_mutex_lock(&mutex);
            clientSock = client_socket;
            client_f = true;
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mutex);        
        }
        else{
            close(client_socket);
        }

    }


}





void* handle_connection(int client_socket,int num){
    int s,d,byte;
    char* res;
    recv(client_socket,&s,sizeof(int),0);
    recv(client_socket,&d,sizeof(int),0);
    char* buf = malloc(sizeof(char)*1024);
    sprintf(buf,"%lu Thread #%d: searching database for a path from node %d to node %d\n",t(),num,s,d);
    while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
    free(buf);
    if( s >= 0 && s < graph->indx &&  d >=0 && d < graph->indx ){
        pthread_mutex_lock(&mutex_db);
        if(database[s].path != NULL){
            if(database[s].d == d){         
                char* p = database[s].path;
                byte = strlen(p);
                
                buf = malloc(sizeof(char)*(1024+byte));
                sprintf(buf,"%lu Thread #%d: path found in database: %s\n",t(),num,p);
                while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
                free(buf);
                
                send(client_socket,&byte,sizeof(int),0);
                send(client_socket,p,byte,0);
                close(client_socket);     
                pthread_mutex_unlock(&mutex_db);
                return NULL;          
            }
            else{
                 CNode* head = database[s].next;
                 while(head != NULL){
                     int dst = head->d;
                     if(head->path != NULL && dst == d){
                        char* p = head->path;
                        byte = strlen(p);

                       buf = malloc(sizeof(char)*(1024+byte));
                        sprintf(buf,"%lu Thread #%d: path found in database: %s\n",t(),num,p);
                        while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
                        free(buf);
                        send(client_socket,&byte,sizeof(int),0);
                        send(client_socket,p,byte,0);
                        close(client_socket);   
                        pthread_mutex_unlock(&mutex_db);  
                        return NULL;                          
                     }
                     head = head->next;
                 }
            }
        }
        pthread_mutex_unlock(&mutex_db);


       buf = malloc(sizeof(char)*1024);
        sprintf(buf,"%lu Thread #%d no path in database, calculating %d->%d\n",t(),num,s,d);
        while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
        free(buf);
        res = findPathWbfs(graph,s,d);
        if(res != NULL ){
            buf = malloc(sizeof(char)*(1024+strlen(res)));
            sprintf(buf,"%lu Thread #%d: path calculated: %s\n",t(),num,res);
            while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
            byte = strlen(res);

             sprintf(buf,"%lu Thread #%d: responding to client and adding path to database\n",t(),num);
             while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));

            free(buf);
            send(client_socket,&byte,sizeof(int),0);
            send(client_socket,res,byte,0);
            close(client_socket);
            pthread_mutex_lock(&mutex_db);
            if(database[s].path != NULL){
                if(database[s].next == NULL){
                    database[s].next = malloc(sizeof(CNode));
                    database[s].next->d = d;
                    database[s].next->path = malloc(sizeof(char)*(byte+1));
                    memcpy( database[s].next->path,res,byte+1);
                    database[s].next->next = NULL;
                }
                else{
                    CNode* head = database[s].next;
                    CNode* prev;
                    while(head != NULL){
                        prev = head;
                        head = head->next;
                    }
                    prev->next = malloc(sizeof(CNode));
                    prev->next->d = d;
                    prev->next->path = malloc(sizeof(char)*(byte+1));           
                    prev->next->next = NULL;
                    memcpy(prev->next->path,res,byte+1);
                }
            }
            else{
                database[s].d = d;
                database[s].path = malloc(sizeof(char)*(byte+1));
                memcpy(database[s].path,res,byte+1);
                database[s].next = NULL;
            }
            pthread_mutex_unlock(&mutex_db);
            free(res);
            return NULL;
        }
    }
    else{
        res = NULL;
    }

    if(res == NULL){
        buf = malloc(sizeof(char)*1024);
        sprintf(buf,"%lu Thread #%d: path not possible from node %d to %d\n",t(),num,s,d);
        while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
        free(buf);
        byte = -1;
        send(client_socket,&byte,sizeof(int),0);
    }


    return NULL;
}

void* thread_function(void *arg){
    int num = *((int *) arg);
    char buf[1024];
    while(true){
        sprintf(buf,"%lu Thread #%d: Waiting for connections\n",t(),num);
        while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
        int clientSock_l;
        pthread_mutex_lock(&mutex);
        
        while(!client_f){
            if(flagh){
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
            pthread_cond_wait(&cond,&mutex);
            clientSock_l = clientSock;
        }

        
        client_f = false;
        
        pthread_mutex_unlock(&mutex);
        if(flagh){
            return NULL;
        }

        pthread_mutex_lock(&mutex_r);
        busy_num++;
        idle_num--;
        float perc = (float)busy_num/(float)pool_size;
        perc = perc*100.0;
        sprintf(buf,"%lu A connection has been delegated to thread id #%d, system load %.3f%%\n",t(),num,perc);
        while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
        pthread_cond_signal(&cond_r);
        pthread_mutex_unlock(&mutex_r);

        handle_connection(clientSock_l,num);

        pthread_mutex_lock(&mutex_r);
        busy_num--;
        idle_num++;
        pthread_cond_signal(&cond_main);
        pthread_mutex_unlock(&mutex_r);       

    }
}



void getInput(int argc, char *argv[],char* path,int* port,char* logfile,int* startup_t,int* max_t){
    	int opt;

    if(argc != 11){
        displayUsage();
        exit(1);
    }

	while( (opt = getopt(argc,argv,"i:p:o:s:x:")) != -1 )
	{
		switch(opt)
		{
			case 'i':
				strcpy(path,optarg);
				break;
			case 'p':
                *port = atoi(optarg);
				break;
			case 'o':
				strcpy(logfile,optarg);
				break;
			case 's':
				*startup_t = atoi(optarg);
				break;
			case 'x':
				*max_t = atoi(optarg);
				break;
			default:
				displayUsage();
				exit(0); //TO-DO

		}
	}
}

void displayUsage(){
    char buf[512];
    sprintf(buf,"Usage: ./server -i pathToFile -p PORT -o pathToLogFile -s 4 -x 24\n");
    while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
}

void loadGraph(Graph* g,int fd){
    char c,n1[11],n2[11];
    int indx1 = 0, indx2 = 0;
    bool flag = true;
    int bytesread;
    while( ((bytesread = read(fd,&c,1)) == -1) && (errno == EINTR));

    while(bytesread > 0){

        if(c == '#'){
            flag = false;
        }
        else if(c == '\n'){
            flag = true;
        }
        else if(flag){

            while( c != '\t'){
                n1[indx1] = c;
                indx1++;
                while( ((bytesread = read(fd,&c,1)) == -1) && (errno == EINTR));
            }
            n1[indx1] = '\0';

            while( ((bytesread = read(fd,&c,1)) == -1) && (errno == EINTR));
            while( c != '\n'){
                n2[indx2] = c;
                indx2++;
                while( ((bytesread = read(fd,&c,1)) == -1) && (errno == EINTR));
            }            
            n2[indx2] = '\0';
            indx2 = 0;
            indx1 = 0;
            int node1 = atoi(n1);
            int node2 = atoi(n2); 
            addEdge(g,node1,node2);    
            g->edge++;       
        }
        while( ((bytesread = read(fd,&c,1)) == -1) && (errno == EINTR));
    }

}

void* resizer(void *arg){
    float perc;
    char buf[1024];
    while(true){

        pthread_mutex_lock(&mutex_r);
        perc = (float) busy_num/ (float) pool_size;
        while(perc < 0.75){
            if(flagh){
                pthread_mutex_unlock(&mutex_r);
                return NULL;
            }
            pthread_cond_wait(&cond_r,&mutex_r);
            perc = (float) busy_num/ (float) pool_size;
            
        }

        if(pool_size == max_thrd){

        }
        else{
            int resize = ceil(pool_size + pool_size*0.25);
            if(resize > max_thrd)
                resize = max_thrd;
            int new_indx = pool_size;
            perc = perc*100.0;
            sprintf(buf,"%lu System load %.2f%%, pool extended to %d threads\n",t(),perc,resize-pool_size);
            while(write(logfd,buf,strlen(buf)) == -1 && (errno == EINTR));
            pool_size = resize;
            thread_pool = (pthread_t*) realloc(thread_pool,pool_size*sizeof(pthread_t));
            for (int i = new_indx; i < pool_size; i++)
            {
                pthread_create(&thread_pool[i],NULL,&thread_function,(void*) &i);
            }
        }

        pthread_mutex_unlock(&mutex_r);
    }

}

unsigned long t(){
    return (unsigned long)time(NULL);
}

void freeDatabase(){
    for (int i = 0; i < graph->indx; i++)
    {
        if(database[i].path != NULL){
            free(database[i].path);
        }
        if(database[i].next != NULL){
            CNode* head = database[i].next;
            CNode* prev;
            while(head != NULL){
                prev = head;
                if(head->path != NULL){
                    free(head->path);
                }
                head = head->next;
                free(prev);
            }
        }
    }
    free(database);
}
