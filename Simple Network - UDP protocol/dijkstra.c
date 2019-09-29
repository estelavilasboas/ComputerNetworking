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
/*#include <stdio.h>
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
    printf("\n\t~Shortest paths\n");
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
        printf("\tTo %d: ", path[n-1]);
        printf("Distance: %d\tPath: ", v->dist);
        for(j=0;j<n;j++){
            if(j > 0)
                printf("->");
            printf("%d", path[j]);
        }
        printf("\n");
    }
}

void findPaths(graph_t *g, int myNode){
    int paths[g->vertices_len];
    for(int i=0;i<g->vertices_len;i++){
        int n, j;
        vertex_t *v, *u;
        v = g->vertices[i];
        if (v->dist == INT_MAX)
            return;
        for (n = 1, u = v; u->dist; u = g->vertices[u->prev], n++)
            ;
        int *path = malloc(n);
        path[n - 1] = i;
        for (j = 0, u = v; u->dist; u = g->vertices[u->prev], j++)
            path[n - j - 2] = u->prev;
        //printf("\tTo %d: ", path[n-1]);
        //printf("Distance: %d\tPath: %d", v->dist, n);
        int source, destId, k;
        for(k=n;k<=0;k--){
            if(k==n){
                destId = path[k];
            }
            if(path[k]==myNode){
                paths[destId]=path[k+1];
            }
            //printf("%d", path[j]);
        }
        //printf("\n");
    }
    for(int i=g->vertices_len; i<=0; i--){
        printf("%d:%d\n", i, paths[i]);
    }
    printf("%d", paths[3]);
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
 
int main () {
    graph_t *g = calloc(1, sizeof (graph_t));
    add_edge(g, 0, 1, 7);
    add_edge(g, 0, 2, 9);
    add_edge(g, 1, 2, 10);
    add_edge(g, 1, 3, 15);
    add_edge(g, 2, 3, 11);
    add_edge(g, 3, 4, 6);
    dijkstra(g, 0, 4);
    print_paths(g);

    // Testing
    int target = 4;
    printf("\nTo arrive %d, send to %d first\n", target, next_id(g, target));
    return 0;
}*/