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

int dv[MAX_NODES][MAX_NODES];       // dv[u][v] = cost from u to v
int nextHop[MAX_NODES][MAX_NODES]; 

void print_forwarding_table(FILE* fp) {
    for (int u = 0; u < num_nodes; u++) {
        for (int d = 0; d < num_nodes; d++) {
            if (dv[u][d] != INF) {
                fprintf(fp, "%d %d %d\n", nodes[d], nextHop[u][d], dv[u][d]);
            }
        }
        fprintf(fp, "\n"); // optional blank line between nodes
    }
}



void distance_vector_update() {
    int updated;
    do {
        updated = 0;

        for (int u = 0; u < num_nodes; u++) {          // source node u
            for (int d = 0; d < num_nodes; d++) {      // destination d
                if (u == d) continue;

                int bestCost = dv[u][d];
                int bestNextHop = nextHop[u][d];

                for (int n = 0; n < num_nodes; n++) {  // neighbor n
                    if (graph[u][n] == INF || n == u) continue;

                    int costViaN = graph[u][n] + dv[n][d];

                    if (costViaN < bestCost || 
                       (costViaN == bestCost && nodes[n] < bestNextHop)) {
                        bestCost = costViaN;
                        bestNextHop = nodes[n];
                    }
                }

                if (bestCost != dv[u][d] || bestNextHop != nextHop[u][d]) {
                    dv[u][d] = bestCost;
                    nextHop[u][d] = bestNextHop;
                    updated = 1;
                }
            }
        }

    } while (updated);  // repeat until no changes
}

void route_messages(FILE* fp) {
    for (int m = 0; m < num_messages; m++) {
        int srcID = messages[m].src;
        int dstID = messages[m].dest;
        char* msg = messages[m].text;

        // Find indices
        int u = -1, d = -1;
        for (int i = 0; i < num_nodes; i++) {
            if (nodes[i] == srcID) u = i;
            if (nodes[i] == dstID) d = i;
        }

        if (u == -1 || d == -1 || dv[u][d] == INF) {
            fprintf(fp, "from %d to %d cost infinite hops unreachable message %s\n", srcID, dstID, msg);
            continue;
        }

        int cost = dv[u][d];
        fprintf(fp, "from %d to %d cost %d hops %d", srcID, dstID, cost, srcID);

        int cur = u;
        while (nodes[cur] != dstID) {
            int nextHopID = nextHop[cur][d];
            if (nextHopID == -1) {
                // Path is unreachable (shouldn't happen if dv is not INF, but safety check)
                break;
            }
            if(nextHopID != dstID) fprintf(fp, " %d", nextHopID);
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
        }
        fprintf(fp, " message %s\n", msg);
    }
}

void apply_change(Change c) {
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
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }
    read_topology(argv[1]);
    read_messages(argv[2]);
    read_changes(argv[3]);

    FILE *fpOut;
    fpOut = fopen("output.txt", "w");

    for (int i = 0; i < num_nodes; i++) {
        for (int j = 0; j < num_nodes; j++) {
            dv[i][j] = (i == j) ? 0 : INF;   // distance to self = 0, others = INF
            nextHop[i][j] = -1;              // -1 means unknown/unreachable
        }
    }
    // Initialize distance vector with direct link costs
    for (int i = 0; i < num_nodes; i++) {
        for (int j = 0; j < num_nodes; j++) {
            if (graph[i][j] != INF && i != j) {
                dv[i][j] = graph[i][j];
                nextHop[i][j] = nodes[j];  // node ID, not index
            }
        }
        dv[i][i] = 0;
        nextHop[i][i] = nodes[i];
    }

    distance_vector_update();
    print_forwarding_table(fpOut);
    route_messages(fpOut);

    for(int c = 0; c < num_changes; c++) {
        apply_change(changes[c]);
        
        // Reinitialize distance vectors after topology change
        for (int i = 0; i < num_nodes; i++) {
            for (int j = 0; j < num_nodes; j++) {
                dv[i][j] = (i == j) ? 0 : INF;
                nextHop[i][j] = -1;
            }
        }
        // Initialize with new direct link costs
        for (int i = 0; i < num_nodes; i++) {
            for (int j = 0; j < num_nodes; j++) {
                if (graph[i][j] != INF && i != j) {
                    dv[i][j] = graph[i][j];
                    nextHop[i][j] = nodes[j];
                }
            }
            dv[i][i] = 0;
            nextHop[i][i] = nodes[i];
        }
        
        distance_vector_update();
        print_forwarding_table(fpOut);
        route_messages(fpOut);
    }
    fclose(fpOut);

    return 0;
}

