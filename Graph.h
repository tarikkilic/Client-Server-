#ifndef GRAPH
#define GRAPH

#define VECTOR_INIT_CAP 5

typedef struct Node{
    int data, destSize, destIndx;
     struct Node * dests;
} Node;


typedef struct Graph{
    struct  Node* nodes;
    int size;
    int indx;
    int edge;
} Graph;

typedef struct qnode{
    int* data;
    int size;
    struct qnode* next;
}qnode;

typedef struct Queue{
    struct qnode* tail;
    struct qnode* head;
}Queue;


//Graph function
Graph* graph_create();
void addEdge(Graph* graph, int node1, int node2);
Node createNode(int data);
void addDest(Graph* graph, int node1, int node2);
void printGraph(Graph* g);
void freeGraph(Graph* g);
char* findPathWbfs(Graph* g, int s, int d);
void initNode(Graph* g,int data);

void c_queue(Queue* q);
void enqueue(Queue* q, int* path,int size);
void dequeue(Queue* q);
char* pathToChar(int* path,int size);
void freeQueue(Queue* q);


#endif