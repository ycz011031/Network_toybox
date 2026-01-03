# Network Toybox

A comprehensive collection of networking implementations and protocols for educational purposes. This repository contains implementations of fundamental networking concepts from the UIUC ECE 438 course, including socket programming, HTTP communication, reliable transport protocols, routing algorithms, and medium access control protocols.

## Repository Structure

The repository is organized into the following directories:

### 1. **csma/** - CSMA Protocol Simulation
Implements Carrier Sense Multiple Access (CSMA) protocol simulation.

- **`src/csma.cpp`**: Main CSMA protocol simulator
  - Simulates CSMA/CD (Collision Detection) behavior
  - Models multiple nodes competing for channel access
  - Implements exponential backoff for collision handling
  - Tracks collision counts and transmission times
  - Reads configuration from input files specifying network parameters (N nodes, L frame length, M backoff limit, R backoff sequence, T total simulation time)

- **`input.txt`**: Configuration file for CSMA simulation
  - Defines network parameters for the simulation

- **`Makefile`**: Build configuration
  - Compiles the CSMA simulator using g++ with pthread support

---

### 2. **sockets/** - Basic Socket Programming
Fundamental socket programming examples demonstrating TCP and UDP communication.

- **`src/server.c`**: TCP stream socket server
  - Demonstrates TCP server implementation
  - Reads and serves file contents to clients
  - Handles multiple connections (fork-based approach)
  - Implements signal handling for child processes

- **`src/client.c`**: TCP stream socket client
  - Connects to TCP servers and receives data
  - Supports both IPv4 and IPv6
  - Implements the client-server communication pattern

- **`src/talker.c`**: UDP datagram client
  - Sends UDP datagrams to a listener
  - Example of connectionless communication
  - Takes hostname and message as arguments

- **`src/listener.c`**: UDP datagram server
  - Listens for incoming UDP datagrams
  - Implements connectionless server pattern
  - Supports both IPv4 and IPv6

- **`Makefile`**: Build configuration
  - Compiles all socket programs (server, client, talker, listener)

---

### 3. **http_servers/** - HTTP Protocol Implementation
Custom HTTP server and client implementations.

- **`src/http_server.c`**: Custom HTTP server
  - Implements basic HTTP protocol support
  - Handles HTTP requests on a specified port
  - Uses socket API for network communication
  - Parses command-line port argument for server binding

- **`src/http_client.c`**: HTTP client
  - Implements HTTP GET request functionality
  - Parses URLs and extracts hostname, port, and file path
  - Supports custom port specification
  - Handles HTTP response parsing
  - Connects to HTTP servers and retrieves resources

- **`Makefile`**: Build configuration
  - Compiles http_server and http_client executables

---

### 4. **reliable_transport_protocol/** - Reliable Data Transfer
Implements a reliable transport protocol over UDP with acknowledgments and retransmission.

- **`src/sender_main.c`**: Reliable UDP sender
  - Implements selective repeat/sliding window protocol
  - Defines packet structure with sequence numbers and payload
  - Key features:
    - MSS (Maximum Segment Size): 1400 bytes
    - MAX_L: Maximum 128 in-flight unacknowledged packets
    - SST (Slow Start Threshold): 64
    - Timeout: 100ms for retransmission
    - Sequence number tracking
    - ACK processing and timeout handling

- **`src/receiver_main.c`**: Reliable UDP receiver
  - Implements receiver-side protocol handling
  - Parses incoming packets with sequence numbers
  - Sends ACKs for received packets
  - Writes payload to output file
  - Handles out-of-order packet delivery
  - Maintains expected sequence number state

- **`Makefile`**: Build configuration
  - Compiles reliable_sender and reliable_receiver executables
  - Links pthread library for multi-threaded operation

---

### 5. **routing_algo/** - Routing Algorithms
Implements two fundamental routing algorithms: Distance Vector and Link State.

- **`src/distvec.cpp`**: Distance Vector Routing
  - Implements distributed Bellman-Ford algorithm
  - Reads network topology from file
  - Processes routing updates/changes
  - Computes shortest paths using distance vectors
  - Handles dynamic topology changes
  - Supports up to 100 nodes and 1000 messages

- **`src/linkstate.cpp`**: Link State Routing (Dijkstra's Algorithm)
  - Implements Dijkstra's shortest path algorithm
  - Global knowledge of network topology
  - Computes shortest paths from source to all destinations
  - Reads topology and messages from files
  - Updates routing information based on topology changes
  - Supports up to 100 nodes and 1000 messages

- **`topofile`**: Network topology file
  - Defines network graph with edges and costs
  - Format: source destination cost

- **`messagefile`**: Messages to route file
  - Contains messages to be routed through the network
  - Format: source destination message_text

- **`changesfile`**: Topology changes file
  - Dynamic updates to network topology
  - Format: source destination new_cost

- **`Makefile`**: Build configuration
  - Compiles linkstate and distvec routing algorithms using g++

---

## Building the Project

Each subdirectory contains its own `Makefile`. To build individual modules:

```bash
cd <module_directory>
make
```

To clean build artifacts:

```bash
cd <module_directory>
make clean
```

### Build Requirements
- **GCC/G++**: C and C++ compiler
- **POSIX Socket API**: For network-related modules
- **pthread**: For thread support (required for certain modules)

---

## Module Descriptions

### Network Protocols & Concepts Covered

1. **CSMA Protocol**: Medium access control with collision detection and exponential backoff
2. **Socket Programming**: TCP/UDP socket APIs for network communication
3. **HTTP Protocol**: Application-layer hypertext transfer protocol
4. **Reliable Transport**: Connection-less reliable delivery using sequence numbers and ACKs
5. **Routing Algorithms**: 
   - Distance Vector (Bellman-Ford)
   - Link State (Dijkstra's)

---

## Usage Examples

### CSMA Simulation
```bash
cd csma
make
./csma input.txt
```

### Socket Programming
```bash
cd sockets
make
./server <filename>      # Terminal 1
./client localhost       # Terminal 2
```

### HTTP Communication
```bash
cd http_servers
make
./http_server 8080       # Terminal 1
./http_client http://localhost:8080/file.txt  # Terminal 2
```

### Reliable Transport Protocol
```bash
cd reliable_transport_protocol
make
./reliable_receiver 5000 output.txt       # Terminal 1
./reliable_sender <dest_host> 5000 input_file  # Terminal 2
```

### Routing Algorithms
```bash
cd routing_algo
make
./linkstate                # Runs link state algorithm
./distvec                  # Runs distance vector algorithm
```

---

## Key Implementation Details

### CSMA
- Exponential backoff with configurable parameters
- Collision detection and recovery
- Multi-node network simulation

### Reliable Transport Protocol
- Sliding window protocol (selective repeat)
- Sequence numbering for in-order delivery
- ACK-based retransmission with timeout
- UDP as underlying transport

### Routing Algorithms
- Distance Vector: Distributed computation using Bellman-Ford
- Link State: Centralized computation using Dijkstra's algorithm
- Support for dynamic topology updates

---

## Educational Purpose

This repository serves as a learning resource for:
- Network protocol design and implementation
- Socket programming fundamentals
- Reliable data transfer mechanisms
- Routing in computer networks
- Medium access control protocols

---

## Author Information

Created for UIUC ECE 438 - Computer Networking course

---

## Notes

- All modules compile with standard POSIX socket APIs and are designed for Unix/Linux environments
- Some modules may require root privileges for certain operations
- The implementation focuses on educational clarity rather than production optimization
- Makefiles use standard build conventions with object files stored in `obj/` directory
