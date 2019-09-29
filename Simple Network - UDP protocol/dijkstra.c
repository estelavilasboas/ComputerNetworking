/*#include <stdio.h> 
#include <stdlib.h> 
#include <stdbool.h> 
#include <limits.h> 

// A structure to represent a node in adjacency list 
struct AdjListNode 
{ 
    int dest; 
    int weight; 
    struct AdjListNode* next; 
}; 

// A structure to represent an adjacency liat 
struct AdjList 
{ 
    struct AdjListNode *head; // pointer to head node of list 
}; 

// A structure to represent a graph. A graph is an array of adjacency lists. 
// Size of array will be V (number of vertices in graph) 
struct Graph 
{ 
    int V; 
    struct AdjList* array; 
}; 

// A utility function to create a new adjacency list node 
struct AdjListNode* newAdjListNode(int dest, int weight) 
{ 
    struct AdjListNode* newNode = 
            (struct AdjListNode*) malloc(sizeof(struct AdjListNode)); 
    newNode->dest = dest; 
    newNode->weight = weight; 
    newNode->next = NULL; 
    return newNode; 
} 

// A utility function that creates a graph of V vertices 
struct Graph* createGraph(int V) 
{ 
    struct Graph* graph = (struct Graph*) malloc(sizeof(struct Graph)); 
    graph->V = V; 

    // Create an array of adjacency lists. Size of array will be V 
    graph->array = (struct AdjList*) malloc(V * sizeof(struct AdjList)); 

    // Initialize each adjacency list as empty by making head as NULL 
    for (int i = 0; i < V; ++i) 
        graph->array[i].head = NULL; 

    return graph; 
} 
    }
} 

// Adds an edge to an undirected graph 
void addEdge(struct Graph* graph, int src, int dest, int weight) 
{ 
    // Add an edge from src to dest. A new node is added to the adjacency 
    // list of src. The node is added at the beginning 
    struct AdjListNode* newNode = newAdjListNode(dest, weight); 
    newNode->next = graph->array[src].head; 
    graph->array[src].head = newNode; 

    // Since graph is undirected, add an edge from dest to src also 
    newNode = newAdjListNode(src, weight); 
    newNode->next = graph->array[dest].head; 
    graph->array[dest].head = newNode; 
} 
    }
} 

// Structure to represent a min heap node 
struct MinHeapNode 
{ 
    int v; 
    int dist; 
}; 

// Structure to represent a min heap 
struct MinHeap 
{ 
    int size;    // Number of heap nodes present currently 
    int capacity; // Capacity of min heap 
    int *pos;    // This is needed for decreaseKey() 
    struct MinHeapNode **array; 
}; 

// A utility function to create a new Min Heap Node 
struct MinHeapNode* newMinHeapNode(int v, int dist) 
{ 
    struct MinHeapNode* minHeapNode = 
        (struct MinHeapNode*) malloc(sizeof(struct MinHeapNode)); 
    minHeapNode->v = v; 
    minHeapNode->dist = dist; 
    return minHeapNode; 
} 

// A utility function to create a Min Heap 
struct MinHeap* createMinHeap(int capacity) 
{ 
    struct MinHeap* minHeap = 
        (struct MinHeap*) malloc(sizeof(struct MinHeap)); 
    minHeap->pos = (int *)malloc(capacity * sizeof(int)); 
    minHeap->size = 0; 
    minHeap->capacity = capacity; 
    minHeap->array = 
        (struct MinHeapNode**) malloc(capacity * sizeof(struct MinHeapNode*)); 
    return minHeap; 
} 
    }
} 

// A utility function to swap two nodes of min heap. Needed for min heapify 
void swapMinHeapNode(struct MinHeapNode** a, struct MinHeapNode** b) 
{ 
    struct MinHeapNode* t = *a; 
    *a = *b; 
    *b = t; 
} 

// A standard function to heapify at given idx 
// This function also updates position of nodes when they are swapped. 
// Position is needed for decreaseKey() 
void minHeapify(struct MinHeap* minHeap, int idx) 
{ 
    int smallest, left, right; 
    smallest = idx; 
    left = 2 * idx + 1; 
    right = 2 * idx + 2; 

    if (left < minHeap->size && 
        minHeap->array[left]->dist < minHeap->array[smallest]->dist ) 
    smallest = left; 

    if (right < minHeap->size && 
        minHeap->array[right]->dist < minHeap->array[smallest]->dist ) 
    smallest = right; 

    if (smallest != idx) 
    { 
        // The nodes to be swapped in min heap 
        struct MinHeapNode *smallestNode = minHeap->array[smallest]; 
        struct MinHeapNode *idxNode = minHeap->array[idx]; 

        // Swap positions 
        minHeap->pos[smallestNode->v] = idx; 
        minHeap->pos[idxNode->v] = smallest; 

        // Swap nodes 
        swapMinHeapNode(&minHeap->array[smallest], &minHeap->array[idx]); 

        minHeapify(minHeap, smallest); 
    } 
} 

// A utility function to check if the given minHeap is ampty or not 
int isEmpty(struct MinHeap* minHeap) 
{ 
    return minHeap->size == 0; 
} 

// Standard function to extract minimum node from heap 
struct MinHeapNode* extractMin(struct MinHeap* minHeap) 
{ 
    if (isEmpty(minHeap)) 
        return NULL; 

    // Store the root node 
    struct MinHeapNode* root = minHeap->array[0]; 

    // Replace root node with last node 
    struct MinHeapNode* lastNode = minHeap->array[minHeap->size - 1]; 
    minHeap->array[0] = lastNode; 

    // Update position of last node 
    minHeap->pos[root->v] = minHeap->size-1; 
    minHeap->pos[lastNode->v] = 0; 

    // Reduce heap size and heapify root 
    --minHeap->size; 
    minHeapify(minHeap, 0); 

    return root; 
} 

// Function to decreasy dist value of a given vertex v. This function 
// uses pos[] of min heap to get the current index of node in min heap 
void decreaseKey(struct MinHeap* minHeap, int v, int dist) 
{ 
    // Get the index of v in heap array 
    int i = minHeap->pos[v]; 

    // Get the node and update its dist value 
    minHeap->array[i]->dist = dist; 

    // Travel up while the complete tree is not hepified. 
    // This is a O(Logn) loop 
    while (i && minHeap->array[i]->dist < minHeap->array[(i - 1) / 2]->dist) 
    { 
        // Swap this node with its parent 
        minHeap->pos[minHeap->array[i]->v] = (i-1)/2; 
        minHeap->pos[minHeap->array[(i-1)/2]->v] = i; 
        swapMinHeapNode(&minHeap->array[i], &minHeap->array[(i - 1) / 2]); 

        // move to parent index 
        i = (i - 1) / 2; 
    } 
} 

// A utility function to check if a given vertex 
// 'v' is in min heap or not 
bool isInMinHeap(struct MinHeap *minHeap, int v) 
{ 
if (minHeap->pos[v] < minHeap->size) 
    return true; 
return false; 
} 
            }
} 

// Function to print shortest 
// path from source to j 
// using parent array 
void printPath(int parent[], int j) 
{ 
      
    // Base Case : If j is source 
    if (parent[j] == - 1) 
        return; 
  
    printPath(parent, parent[j]); 
  
    printf("%d ", j); 
} 
        }
} 
  
// A utility function to print  
// the constructed distance 
// array 
int printSolution(int dist[], int V, int parent[]) 
{ 
    int src = 0; 
    printf("Vertex\tDist\tPath"); 
    for (int i = 1; i < V; i++) 
    { 
        printf("\n%d -> %d\t%d\t%d ", src, i, dist[i], src); 
        printPath(parent, i); 
    } 
} 

// The main function that calulates distances of shortest paths from src to all 
// vertices. It is a O(ELogV) function 
void dijkstra(struct Graph* graph, int src) 
{ 
    int V = graph->V;// Get the number of vertices in graph 
    int dist[V];     // dist values used to pick minimum weight edge in cut 
    int previous[V];

    for(int i=0;i<V;i++)
        previous[i] = -1;

    // minHeap represents set E 
    struct MinHeap* minHeap = createMinHeap(V); 

    // Initialize min heap with all vertices. dist value of all vertices 
    for (int v = 0; v < V; ++v) 
    { 
        dist[v] = INT_MAX; 
        minHeap->array[v] = newMinHeapNode(v, dist[v]); 
        minHeap->pos[v] = v; 
    } 
        }
    } 

    // Make dist value of src vertex as 0 so that it is extracted first 
    minHeap->array[src] = newMinHeapNode(src, dist[src]); 
    minHeap->pos[src] = src; 
    dist[src] = 0; 
    decreaseKey(minHeap, src, dist[src]); 

    // Initially size of min heap is equal to V 
    minHeap->size = V; 

    // In the followin loop, min heap contains all nodes 
    // whose shortest distance is not yet finalized. 
    while (!isEmpty(minHeap)) 
    { 
        // Extract the vertex with minimum distance value 
        struct MinHeapNode* minHeapNode = extractMin(minHeap); 
        int u = minHeapNode->v; // Store the extracted vertex number 

        // Traverse through all adjacent vertices of u (the extracted 
        // vertex) and update their distance values 
        struct AdjListNode* pCrawl = graph->array[u].head; 
        while (pCrawl != NULL) 
        { 
            int v = pCrawl->dest; 

            // If shortest distance to v is not finalized yet, and distance to v 
            // through u is less than its previously calculated distance 
            if (isInMinHeap(minHeap, v) && dist[u] != INT_MAX && 
                                        pCrawl->weight + dist[u] < dist[v]) 
            { 
                dist[v] = dist[u] + pCrawl->weight; 
                previous[v] = u;

                // update distance value in min heap also 
                decreaseKey(minHeap, v, dist[v]); 
            } 
        }
            } 
            pCrawl = pCrawl->next; 
        } 
    }
        } 
    } 
}
    } 

    printSolution(dist, V, previous); 
}

int main () {
    graph_t *g = calloc(1, sizeof (graph_t));
    add_edge(g, 0, 1, 7);
    add_edge(g, 0, 2, 9);
    add_edge(g, 0, 5, 14);
    add_edge(g, 1, 2, 10);
    add_edge(g, 1, 3, 15);
    add_edge(g, 2, 3, 11);
    add_edge(g, 2, 5, 2);
    add_edge(g, 3, 4, 6);
    add_edge(g, 4, 5, 9);
    dijkstra(g, 0, 4);
    print_paths(g);

    return 0; 
} */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
 
typedef struct {
    int vertex;
    int weight;
} edge_t;
 
typedef struct {
    edge_t **edges;
    int edges_len;
    int edges_size;
    int dist;
    int prev;
    int visited;
} vertex_t;
 
typedef struct {
    vertex_t **vertices;
    int vertices_len;
    int vertices_size;
} graph_t;
 
typedef struct {
    int *data;
    int *prio;
    int *index;
    int len;
    int size;
} heap_t;

typedef struct {
    int target;
    int next;
    int weight;
} router_t;
 
void add_vertex (graph_t *g, int i) {
    if (g->vertices_size < i + 1) {
        int size = g->vertices_size * 2 > i ? g->vertices_size * 2 : i + 4;
        g->vertices = realloc(g->vertices, size * sizeof (vertex_t *));
        for (int j = g->vertices_size; j < size; j++)
            g->vertices[j] = NULL;
        g->vertices_size = size;
    }
    if (!g->vertices[i]) {
        g->vertices[i] = calloc(1, sizeof (vertex_t));
        g->vertices_len++;
    }
}
 
void add_edge (graph_t *g, int a, int b, int w) {
    add_vertex(g, a);
    add_vertex(g, b);
    vertex_t *v = g->vertices[a];
    if (v->edges_len >= v->edges_size) {
        v->edges_size = v->edges_size ? v->edges_size * 2 : 4;
        v->edges = realloc(v->edges, v->edges_size * sizeof (edge_t *));
    }
    edge_t *e = calloc(1, sizeof (edge_t));
    e->vertex = b;
    e->weight = w;
    v->edges[v->edges_len++] = e;
}
 
heap_t *create_heap (int n) {
    heap_t *h = calloc(1, sizeof (heap_t));
    h->data = calloc(n + 1, sizeof (int));
    h->prio = calloc(n + 1, sizeof (int));
    h->index = calloc(n, sizeof (int));
    return h;
}
 
void push_heap (heap_t *h, int v, int p) {
    int i = h->index[v] == 0 ? ++h->len : h->index[v];
    int j = i / 2;
    while (i > 1) {
        if (h->prio[j] < p)
            break;
        h->data[i] = h->data[j];
        h->prio[i] = h->prio[j];
        h->index[h->data[i]] = i;
        i = j;
        j = j / 2;
    }
    h->data[i] = v;
    h->prio[i] = p;
    h->index[v] = i;
}
 
int min (heap_t *h, int i, int j, int k) {
    int m = i;
    if (j <= h->len && h->prio[j] < h->prio[m])
        m = j;
    if (k <= h->len && h->prio[k] < h->prio[m])
        m = k;
    return m;
}
 
int pop_heap (heap_t *h) {
    int v = h->data[1];
    int i = 1;
    while (1) {
        int j = min(h, h->len, 2 * i, 2 * i + 1);
        if (j == h->len)
            break;
        h->data[i] = h->data[j];
        h->prio[i] = h->prio[j];
        h->index[h->data[i]] = i;
        i = j;
    }
    h->data[i] = h->data[h->len];
    h->prio[i] = h->prio[h->len];
    h->index[h->data[i]] = i;
    h->len--;
    return v;
}
 
void dijkstra (graph_t *g, int a, int b) {
    int i, j;
    for (i = 0; i < g->vertices_len; i++) {
        vertex_t *v = g->vertices[i];
        v->dist = INT_MAX;
        v->prev = 0;
        v->visited = 0;
    }
    vertex_t *v = g->vertices[a];
    v->dist = 0;
    heap_t *h = create_heap(g->vertices_len);
    push_heap(h, a, v->dist);
    while (h->len) {
        i = pop_heap(h);
        if (i == b)
            break;
        v = g->vertices[i];
        v->visited = 1;
        for (j = 0; j < v->edges_len; j++) {
            edge_t *e = v->edges[j];
            vertex_t *u = g->vertices[e->vertex];
            if (!u->visited && v->dist + e->weight <= u->dist) {
                u->prev = i;
                u->dist = v->dist + e->weight;
                push_heap(h, e->vertex, u->dist);
            }
        }
    }
}

void print_paths (graph_t *g) {
    printf("Shortest paths\n");
    for(int i=0;i<g->vertices_len;i++){
        int n, j;
        vertex_t *v, *u;
        v = g->vertices[i];
        if (v->dist == INT_MAX) {
            printf("no path\n");
            return;
        }
        for (n = 1, u = v; u->dist; u = g->vertices[u->prev], n++)
            ;
        int *path = malloc(n);
        path[n - 1] = i;
        for (j = 0, u = v; u->dist; u = g->vertices[u->prev], j++)
            path[n - j - 2] = u->prev;
        printf("To %d: ", path[n-1]);
        printf("Distance: %d\tPath: ", v->dist);
        for(j=0;j<n;j++){
            if(j > 0)
                printf("->");
            printf("%d", path[j]);
        }
        printf("\n");
    }
}
 
int next_id (graph_t *g, int target) {
    int n, j;
    vertex_t *v, *u;
    v = g->vertices[target];
    if (v->dist == INT_MAX)
        return -1;
    for (n = 1, u = v; u->dist; u = g->vertices[u->prev], n++)
        ;
    int *path = malloc(n);
    path[n - 1] = target;
    for (j = 0, u = v; u->dist; u = g->vertices[u->prev], j++)
        path[n - j - 2] = u->prev;
    if (n == 1)
        return path[0];
    return path[1];
}
 /*
int main () {
    graph_t *g = calloc(1, sizeof (graph_t));
    add_edge(g, 0, 1, 7);
    add_edge(g, 0, 2, 9);
    add_edge(g, 0, 5, 14);
    add_edge(g, 1, 2, 10);
    add_edge(g, 1, 3, 15);
    add_edge(g, 2, 3, 11);
    add_edge(g, 2, 5, 2);
    add_edge(g, 3, 4, 6);
    add_edge(g, 4, 5, 9);
    dijkstra(g, 0, 4);
    print_paths(g);

    // Testing
    int target = 4;
    printf("\nTo arrive %d, send to %d first\n", target, next_id(g, target));
    return 0;
}
*/