#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#define MAX_NODES 100
#define MAX_MSGS 1000
#define INF 999999

typedef struct {
    int src;
    int dest;
    int cost;
} Edge;

typedef struct
{
    int src;
    int dest;
    char text[256];

} Message;

typedef struct {
    int src;
    int dest;
    int cost;
} Change;

int graph[MAX_NODES][MAX_NODES];
int nodes[MAX_NODES];
int num_nodes = 0;

int addNode(int node) {
    for (int i = 0; i < num_nodes; i++) {
        if (nodes[i] == node) {
            return i;
        }
    }
    nodes[num_nodes] = node;
    return num_nodes++;
}

void read_topology(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open topology file");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_NODES; i++) {
        for (int j = 0; j < MAX_NODES; j++) {
            graph[i][j] = (i == j) ? 0 : INF;
        }
    }

    int src, dest, cost;
    while (fscanf(file, "%d %d %d", &src, &dest, &cost) != EOF) {
        int srcIndex = addNode(src);
        int destIndex = addNode(dest);
        graph[srcIndex][destIndex] = cost;
        graph[destIndex][srcIndex] = cost;
    }

    fclose(file);

    // Sort nodes array to ensure consistent ordering
    for (int i = 0; i < num_nodes - 1; i++) {
        for (int j = i + 1; j < num_nodes; j++) {
            if (nodes[i] > nodes[j]) {
                int temp = nodes[i];
                nodes[i] = nodes[j];
                nodes[j] = temp;

                // Swap corresponding rows and columns in graph
                for (int k = 0; k < num_nodes; k++) {
                    int tempVal = graph[i][k];
                    graph[i][k] = graph[j][k];
                    graph[j][k] = tempVal;
                }
                for (int k = 0; k < num_nodes; k++) {
                    int tempVal = graph[k][i];
                    graph[k][i] = graph[k][j];
                    graph[k][j] = tempVal;
                }
            }
        }
    }
}

int num_messages = 0;
Message messages[MAX_MSGS];

void read_messages(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open message file");
        exit(EXIT_FAILURE);
    }

    char line[512];
    while (fgets(line, sizeof(line), file) != NULL) {
        if (sscanf(line, "%d %d", &messages[num_messages].src, &messages[num_messages].dest) == 2) {
            // find start of message text
            char* msgText = strchr(line, ' ');
            msgText = strchr(msgText + 1, ' '); 
            if (msgText) {
                while (*msgText == ' ') msgText++; 
                strcpy(messages[num_messages].text, msgText);
                size_t len = strlen(messages[num_messages].text);
                if (messages[num_messages].text[len - 1] == '\n')
                    messages[num_messages].text[len - 1] = '\0';
            }
            num_messages++;
        }
    }

    fclose(file);
}


int num_changes = 0;
Change changes[MAX_MSGS];

void read_changes(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open changes file");
        exit(EXIT_FAILURE);
    }

    int src, dest, cost;
    while (fscanf(file, "%d %d %d", &src, &dest, &cost) != EOF) {
        changes[num_changes].src = src;
        changes[num_changes].dest = dest;
        changes[num_changes].cost = cost;
        num_changes++;
    }

    fclose(file);
}

int dist[MAX_NODES][MAX_NODES];      // dist[u][v] = shortest distance from u to v
int nextHopLS[MAX_NODES][MAX_NODES]; // nextHopLS[u][v] = next hop node ID from u to v

void dijkstra(int srcIndex) {
    int visited[MAX_NODES] = {0};
    int prev[MAX_NODES];   // to track the last node before destination

    for (int i = 0; i < num_nodes; i++) {
        dist[srcIndex][i] = INF;
        nextHopLS[srcIndex][i] = -1;
        prev[i] = -1;
    }

    dist[srcIndex][srcIndex] = 0;
    nextHopLS[srcIndex][srcIndex] = nodes[srcIndex];
    prev[srcIndex] = srcIndex;

    for (int count = 0; count < num_nodes; count++) {
        // pick unvisited node with smallest dist
        int u = -1;
        int minDist = INF + 1;
        for (int i = 0; i < num_nodes; i++) {
            if (!visited[i] && dist[srcIndex][i] < minDist) {
                minDist = dist[srcIndex][i];
                u = i;
            } else if (!visited[i] && dist[srcIndex][i] == minDist && u != -1 && nodes[i] < nodes[u]) {
                u = i; // tie-break lowest node ID
            }
        }

        if (u == -1) break; // no more reachable nodes
        visited[u] = 1;

        // update neighbors
        for (int v = 0; v < num_nodes; v++) {
            if (graph[u][v] == INF) continue;

            int newDist = dist[srcIndex][u] + graph[u][v];

            if (newDist < dist[srcIndex][v]) {
                dist[srcIndex][v] = newDist;
                prev[v] = u;
                // Update next hop: if u is the source, next hop is v; otherwise inherit from u
                if (u == srcIndex) {
                    nextHopLS[srcIndex][v] = nodes[v];
                } else {
                    nextHopLS[srcIndex][v] = nextHopLS[srcIndex][u];
                }
            } else if (newDist == dist[srcIndex][v]) {
                // tie-break: choose path whose last node before destination (u) has smaller node ID
                if (prev[v] == -1 || nodes[u] < nodes[prev[v]]) {
                    prev[v] = u;
                    // Update next hop based on new path
                    if (u == srcIndex) {
                        nextHopLS[srcIndex][v] = nodes[v];
                    } else {
                        nextHopLS[srcIndex][v] = nextHopLS[srcIndex][u];
                    }
                }
            }
        }
    }
}
void compute_linkstate() {
    for (int u = 0; u < num_nodes; u++) {
        // printf("Computing linkstate for node %d\n", nodes[u]);
        dijkstra(u);
    }
}

void print_forwarding_table_ls(FILE* fp) {
    for (int u = 0; u < num_nodes; u++) {
        for (int d = 0; d < num_nodes; d++) {
            if (dist[u][d] != INF) {
                fprintf(fp, "%d %d %d\n", nodes[d], nextHopLS[u][d], dist[u][d]);
            }
        }
        fprintf(fp, "\n"); // optional blank line between nodes
    }
}
void route_messages_ls(FILE* fp) {
    for (int m = 0; m < num_messages; m++) {
        int srcID = messages[m].src;
        int dstID = messages[m].dest;
        char* msg = messages[m].text;

        int u = -1, d = -1;
        for (int i = 0; i < num_nodes; i++) {
            if (nodes[i] == srcID) u = i;
            if (nodes[i] == dstID) d = i;
        }

        if (u == -1 || d == -1 || dist[u][d] == INF) {
            fprintf(fp, "from %d to %d cost infinite hops unreachable message %s\n", srcID, dstID, msg);
            continue;
        }

        int cost = dist[u][d];
        fprintf(fp, "from %d to %d cost %d hops %d", srcID, dstID, cost, srcID);

        int cur = u;
        int hopCount = 0;
        while (cur != d && hopCount < num_nodes) {
            int nextHopID = nextHopLS[cur][d];
            if(nextHopID != dstID ){
                fprintf(fp, " %d", nextHopID);
                //printf("nextHopID %d, with destination %d\n", nextHopID, dstID);
            }
            
            // move to next hop index
            int nextIndex = -1;
            for (int i = 0; i < num_nodes; i++) {
                if (nodes[i] == nextHopID) {
                    nextIndex = i;
                    break;
                }
            }
            if (nextIndex == -1 || nextIndex == cur) break; // safety
            cur = nextIndex;
            hopCount++;
        }
        fprintf(fp, " message %s\n", msg);
    }
}
void apply_change_ls(Change c) {
    int u = -1, v = -1;
    for (int i = 0; i < num_nodes; i++) {
        if (nodes[i] == c.src) u = i;
        if (nodes[i] == c.dest) v = i;
    }
    if (u == -1 || v == -1) return;

    if (c.cost == -999) {
        graph[u][v] = INF;
        graph[v][u] = INF;
    } else {
        graph[u][v] = c.cost;
        graph[v][u] = c.cost;
    }
}



int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }
    // printf("Calling read_topology with file: %s\n", argv[1]);
    read_topology(argv[1]);

    // printf("Calling read_messages with file: %s\n", argv[2]);
    read_messages(argv[2]);

    // printf("Calling read_changes with file: %s\n", argv[3]);
    read_changes(argv[3]);

    FILE *fpOut;
    // printf("Opening output file: output.txt\n");
    fpOut = fopen("output.txt", "w");

    // printf("Calling compute_linkstate\n");
    compute_linkstate();

    // printf("Calling print_forwarding_table_ls\n");
    print_forwarding_table_ls(fpOut);

    // printf("Calling route_messages_ls\n");
    route_messages_ls(fpOut);

    for (int i = 0; i < num_changes; i++) {
        // printf("Applying change %d\n", i + 1);
        // fprintf(fpOut, "——– At this point, %d%s change is applied\n", i + 1,
        //     (i==0)?"st":(i==1)?"nd":(i==2)?"rd":"th");

        apply_change_ls(changes[i]);

        // printf("Recomputing linkstate after change %d\n", i + 1);
        compute_linkstate();

        // printf("Printing forwarding table after change %d\n", i + 1);
        print_forwarding_table_ls(fpOut);

        // printf("Routing messages after change %d\n", i + 1);
        route_messages_ls(fpOut);
    }

    // printf("Closing output file\n");
    fclose(fpOut);
    

    return 0;
}

