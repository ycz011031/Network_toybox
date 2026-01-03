// csma.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int id;
    int backoff;         // current backoff counter
    int collision_count; // collisions for current packet
    int R_index;         // index into R_list
    int transmit_time;   // remaining ticks for ongoing transmission (0 => not transmitting)
    int started_collided; // 1 if this transmission was a collision, 0 if it was "single" start
} Node;

void parse_input(const char *filename,
                 int *N, int *L, int *M, int **R_list, int *R_count, int *T) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error opening input file.\n");
        exit(1);
    }

    char line[256];
    *R_list = NULL;
    *R_count = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '\n' || line[0] == '\r') continue;

        char *p = line;
        while (*p == ' ' || *p == '\t') p++; // skip leading spaces

        if (p[0] == 'N') sscanf(p + 1, "%d", N);
        else if (p[0] == 'L') sscanf(p + 1, "%d", L);
        else if (p[0] == 'M') sscanf(p + 1, "%d", M);
        else if (p[0] == 'R') {
            // Count how many integers are in the line
            int count = 0;
            char *q = p + 1;
            while (*q == ' ' || *q == '\t') q++; // skip leading spaces
            char *temp = q;
            int tmp;
            while (sscanf(temp, "%d", &tmp) == 1) {
                count++;
                while (*temp && *temp != ' ' && *temp != '\t' && *temp != '\n' && *temp != '\r') temp++;
                while (*temp == ' ' || *temp == '\t') temp++;
            }

            if (count == 0) {
                fprintf(stderr, "Error: no R values found\n");
                exit(1);
            }

            *R_list = (int *)malloc(sizeof(int) * count);
            *R_count = count;

            // Read numbers into R_list
            q = p + 1;
            while (*q == ' ' || *q == '\t') q++; // skip leading spaces
            int idx = 0;
            while (idx < count && sscanf(q, "%d", &tmp) == 1) {
                (*R_list)[idx++] = tmp;
                while (*q && *q != ' ' && *q != '\t' && *q != '\n' && *q != '\r') q++;
                while (*q == ' ' || *q == '\t') q++;
            }
        } else if (p[0] == 'T') sscanf(p + 1, "%d", T);
    }

    fclose(fp);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: ./csma input.txt\n");
        return -1;
    }

    int N = 0, L = 0, M = 0, T = 0;
    int *R_list = NULL;
    int R_count = 0;

    parse_input(argv[1], &N, &L, &M, &R_list, &R_count, &T);

    if (N <= 0 || L <= 0 || M <= 0 || R_count <= 0 || T <= 0) {
        fprintf(stderr, "Invalid input parameters.\n");
        if (R_list) free(R_list);
        return -1;
    }

    // Allocate and initialize nodes
    Node *nodes = (Node*)malloc(sizeof(Node) * N);
    if (!nodes) {
        fprintf(stderr, "Memory allocation failed\n");
        free(R_list);
        return -1;
    }

    for (int i = 0; i < N; ++i) {
        nodes[i].id = i;
        nodes[i].collision_count = 0;
        nodes[i].R_index = 0;
        nodes[i].transmit_time = 0;
        nodes[i].started_collided = 0;
        nodes[i].backoff = (nodes[i].id + 0) % R_list[nodes[i].R_index];
    }

    long successful_ticks = 0;

    // Print R_list at the beginning
    // printf("R_list: ");
    // for (int i = 0; i < R_count; ++i) {
    //     printf("%d ", R_list[i]);
    // }
    // printf("\n");

    // Simulation loop
    for (int tick = 0; tick < T; ++tick) {
        // Print backoff values at start of tick
        // printf("Tick %d: ", tick);
        // for (int i = 0; i < N; ++i) {
        //     printf("N%d=%d ", i, nodes[i].backoff);
        // }
        // printf("\n");
        
        // Step 1: Check ongoing transmissions and nodes wanting to start
        int transmitting_count = 0;
        int want_to_start = 0;
        
        for (int i = 0; i < N; ++i) {
            if (nodes[i].transmit_time > 0) {
                transmitting_count++;
            } else if (nodes[i].backoff == 0) {
                want_to_start++;
            }
        }
        
        // Step 2: Handle transmissions and collisions
        if (transmitting_count > 0) {
            // Channel is busy - count if exactly one transmitting
            if (transmitting_count == 1) {
                successful_ticks++;
            }
            // Other nodes freeze (no countdown)
        } else if (want_to_start > 0) {
            // Channel idle, some nodes want to start
            if (want_to_start == 1) {
                // Single node starts - successful
                for (int i = 0; i < N; ++i) {
                    if (nodes[i].backoff == 0) {
                        nodes[i].transmit_time = L;
                        nodes[i].started_collided = 0;
                        break;
                    }
                }
                successful_ticks++;
            } else {
                // Multiple nodes start - collision
                for (int i = 0; i < N; ++i) {
                    if (nodes[i].backoff == 0) {
                        nodes[i].transmit_time = 0;
                        nodes[i].started_collided = 1;
                        nodes[i].collision_count++;                        
                        if (nodes[i].collision_count >= M) {
                            nodes[i].collision_count = 0;
                            nodes[i].R_index = 0;
                        } else if (nodes[i].R_index < R_count - 1) {
                            nodes[i].R_index++;
                        }
                        int Rval = R_list[nodes[i].R_index];
                        nodes[i].backoff = (nodes[i].id + tick+1) % Rval;

                        // Debug message for collision
                        // printf("Collision at tick %d: Node %d collided with Rval=%d, Rindex=%d\n", tick, nodes[i].id, Rval, nodes[i].R_index);
                    }
                }
            }
        } else {
            // Channel idle, nobody wants to start - all count down
            for (int i = 0; i < N; ++i) {
                if (nodes[i].backoff > 0) {
                    nodes[i].backoff--;
                }
            }
        }
        
        // Step 3: At end of tick, decrement transmit_time
        for (int i = 0; i < N; ++i) {
            if (nodes[i].transmit_time > 0) {
                nodes[i].transmit_time--;
                
                // If transmission finished, pick new backoff
                if (nodes[i].transmit_time == 0) {
                    if (nodes[i].started_collided == 0) {
                        nodes[i].collision_count = 0;
                        nodes[i].R_index = 0;
                    }
                    
                    // Pick new backoff using current tick
                    int Rval = R_list[nodes[i].R_index];
                    nodes[i].backoff = (nodes[i].id + tick+1) % Rval;
                    nodes[i].started_collided = 0;

                    // Debug message for transmission completion
                    // printf("Completion at tick %d: Node %d completed transmission with Rval=%d, Rindex=%d\n", tick, nodes[i].id, Rval, nodes[i].R_index);
                }
            }
        }
    }

    double utilization = ((double)successful_ticks) / ((double)T);

    FILE *fpOut = fopen("output.txt", "w");
    if (!fpOut) {
        fprintf(stderr, "Error opening output.txt for writing\n");
        free(nodes);
        free(R_list);
        return -1;
    }
    fprintf(fpOut, "%.2f\n", utilization);
    fclose(fpOut);

    free(nodes);
    free(R_list);
    return 0;
}