#include "Graph.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

Graph* graph_create(){
    
    Graph* graph = (Graph*) malloc(sizeof(Graph)); 
    
    graph->nodes = NULL;
   graph->edge = 0;
    return graph;
}

void addEdge(Graph* graph, int node1, int node2){


    if(graph->nodes == NULL){
        if(node1 > node2){
            graph->nodes = (Node*) calloc((node1+1),sizeof(Node));
            initNode(graph,node1);
            initNode(graph,node2);
            graph->size = node1+1;
            graph->indx = node1+1;
            addDest(graph,node1,node2); 
        }
        else{
            graph->nodes = (Node*) calloc((node2+1),sizeof(Node));
            initNode(graph,node1);
            initNode(graph,node2);
            graph->size = node2+1; 
            graph->indx = node2+1;
            addDest(graph,node1,node2);
        }
    }
    else{
        // printf("node: %d - destIndx: %d\n", graph->nodes[node1.data].data,graph->nodes[node1.data].destIndx);
        if(node1 > node2) {
            if(node1 > graph->size - 1){
                int percfif = ceil(node1 + node1*0.50);
                graph->nodes = (Node*) realloc(graph->nodes,sizeof(Node)*percfif);
                graph->size = percfif;

            }
            if(graph->indx < node1+1)
                graph->indx = node1 + 1;
            // Add Dest
            addDest(graph,node1,node2);                  
        }
        else if(node1 <= node2){
            
            if(node2 > graph->size - 1){
                int percfif = node2 + node2*0.50;
                graph->nodes = (Node*) realloc(graph->nodes,sizeof(Node)*percfif);
                graph->size = percfif;

            }
            // Add Dest
            if(graph->indx < node2+1)
                graph->indx = node2 + 1;           
            addDest(graph,node1,node2);
            
        }
    }
}

void initNode(Graph* g,int data){
    g->nodes[data].data = data;
    g->nodes[data].destSize = 3;
    g->nodes[data].destIndx = 0;
    g->nodes[data].dests = NULL;
}

void addDest(Graph* graph, int node1, int node2){
    //Add Edge
    if(graph->nodes[node1].dests == NULL){
        //node2, node1 in komsu arrayine eklenir

        initNode(graph,node1);
        int x = graph->nodes[node1].destSize;
        graph->nodes[node1].dests = (Node*) malloc(sizeof(Node)*x);
        int index =  graph->nodes[node1].destIndx;
       
       if(graph->nodes[node2].dests == NULL){
            initNode(graph,node2);
            graph->nodes[node2].data = node2;
            int y = graph->nodes[node2].destSize;
            graph->nodes[node2].dests = (Node*) malloc(sizeof(Node)*y);            
        }
        //graph->nodes[node1].dests[index].data = node2;
        graph->nodes[node1].dests[index] = graph->nodes[node2];
        graph->nodes[node1].destIndx +=1;

    }
    else{
        //Eger komsu arrayi dolmus ise arttilir
        
        if(graph->nodes[node1].destIndx == graph->nodes[node1].destSize - 1){
            // printf("sa2\n");
            int new_size = ceil(graph->nodes[node1].destSize + graph->nodes[node1].destSize*0.5);
              //printf("%d\n", graph->nodes[node1].destIndx);

            graph->nodes[node1].dests = (Node*)  realloc(graph->nodes[node1].dests,sizeof(Node)*new_size);
          
            graph->nodes[node1].destSize = new_size;
             
        }
         
        //node2, node1 in komsu arrayine eklenir
        int index =  graph->nodes[node1].destIndx;
        if(graph->nodes[node2].dests == NULL){
            initNode(graph,node2);
            graph->nodes[node2].data = node2;
            int y = graph->nodes[node2].destSize;
            graph->nodes[node2].dests = (Node*) malloc(sizeof(Node)*y);            
        }        
      
        graph->nodes[node1].dests[index] = graph->nodes[node2];
        graph->nodes[node1].destIndx +=1;
    }    
}


void freeGraph(Graph* g){


    for (int i = 0; i < g->indx; i++)
    {
        
        if(g->nodes[i].dests != NULL)
            free(g->nodes[i].dests);
    }
    if(g->nodes != NULL)
        free(g->nodes);
    free(g);
}

void printGraph(Graph* g){

    for (size_t i = 0; i < g->indx; i++)
    {
        if(g->nodes[i].data == i){
            printf("%d: ",g->nodes[i].data);
            if(g->nodes[i].destIndx != 0){
                for (size_t j = 0; j < g->nodes[i].destIndx; j++)
                {
                    printf(" %d -",g->nodes[i].dests[j].data);
                }
            }
            printf("\n");
        }
    }
}



void c_queue(Queue* q){
    q->head = NULL;
    q->tail = NULL;
}

void enqueue(Queue* q, int* path, int size){


    qnode* qn; 
    qn = malloc(sizeof(qnode));   
    qn->data = malloc(sizeof(int)*size);
 
    memcpy(qn->data,path,sizeof(int)*size);
    //free(path);
    qn->next = NULL;
    qn->size = size;

    if(q->tail == NULL){
        q->head = qn;
        q->tail = qn;
        return;
    }
    q->tail->next = qn;
    q->tail = qn;
}

void dequeue(Queue* q){
    if(q->tail == NULL)
        return;

    qnode* qn = q->head;
    q->head = q->head->next;

    if(q->head == NULL)
        q->tail = NULL;

    free(qn->data);
    free(qn);

}

void checkAllQueue(Queue* q,int d){
    qnode* n =  q->head;
    if(n == NULL){
        return;
    }
    while(n !=  NULL){
        int c = n->data[n->size-1];
        if(c == d)
            exit(1);
        n = n->next;
    }
}

char* findPathWbfs(Graph* g, int s, int d){
    
    Queue* q = malloc(sizeof(Queue));
    int* visited = malloc(sizeof(int)*g->indx); 
    memset(visited,0,sizeof(int)*g->indx);
    q->head = NULL;
    q->tail = NULL;
 
    int* path =  (int*) malloc(sizeof(int));
    int size = 0;
    path[size] = g->nodes[s].data;

    size++;

    enqueue(q,path,size);
    free(path);
    while(q->head != NULL){
        
        size = q->head->size;
        path = (int*) malloc(sizeof(int)*size);
        memcpy(path,q->head->data,sizeof(int)*size);
        dequeue(q);
        int last = path[size-1];
        visited[last] = 1;
        if(last == d){
            char* lst_path = pathToChar(path,size);
            free(visited);
            freeQueue(q);
            free(path);
            free(q);
            return lst_path;
        }
        
        
        for (int i = 0; i < g->nodes[last].destIndx; i++)
        {
            if(visited[g->nodes[last].dests[i].data] == 0){
                int* temp = (int*) malloc(sizeof(int)*(size+1));
                memcpy(temp,path,sizeof(int)*size);   
                int temp_size = size+1;
                temp[temp_size-1] = g->nodes[last].dests[i].data;  
                enqueue(q,temp,temp_size);
                free(temp);
            }
        }
        if(path != NULL)
            free(path);  
    }
    free(visited);
    freeQueue(q);
    free(q);
    return NULL;
}

void freeQueue(Queue* q){
    while(q->head != NULL){
        dequeue(q);
    }
}




char* pathToChar(int* path,int size){
    int num = 50;
    char* last_path = (char*) malloc(sizeof(char)*num);
    int off = 0;
    for (int i = 0; i < size; i++)
    {
        if(off >= num-15){
            num = num*2;
            last_path = (char*) realloc(last_path,sizeof(char)*num);
        }

        if(i == size - 1){
            off += sprintf(last_path+off, "%d",path[i]); 
        }
        else{
            off += sprintf(last_path+off, "%d->",path[i]);
        }
        
    }
    return last_path;

}

