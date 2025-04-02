# CLDP Implementation - Assignment 7

## Assignment Details
**Name:** Rishabh  
**Roll Number:** 22CS10058  
**Assignment:** Networks Laboratory - Assignment 7  

This project implements a Custom Lightweight Discovery Protocol (CLDP) using raw sockets in POSIX C. The protocol facilitates node discovery and metadata exchange in a closed network environment.

---

## Build Instructions

### Change in code
Set the IP_SELF headers in both server and client to the ip address of the system you are currently working on.

### Server
gcc cldp_server.c -o server 

### Client
gcc cldp_client.c -o client

### Run Instructions
Since raw sockets require elevated privileges, run the programs with `sudo`:

#### Server:
sudo ./server

#### Client:
sudo ./client [QUERY_TYPE]

- `QUERY_TYPE` can be:
  - `0` for CPU Load
  - `1` for System Time
  - `2` for Hostname

---

## Features

1. **Protocol Specification**
   - **HELLO (0x01):** Server announces its presence every 5 seconds.
   - **QUERY (0x02):** Client requests metadata (CPU Load, System Time, or Hostname).
   - **RESPONSE (0x03):** Server responds with the requested metadata.

2. **Metadata Supported**
   - **CPU Load:** Returns system uptime, load averages, and memory statistics.
   - **System Time:** Returns current timestamp in ISO 8601 format.
   - **Hostname:** Returns the hostname of the server.

3. **Raw Socket Implementation**
   - Manually crafted IP packets using protocol number `253`.
   - Custom header includes message type, payload length, transaction ID.

4. **Threaded Announcements**
   - Server sends HELLO packets every 5 seconds using a dedicated thread.

---

## Demo Scenario

### Steps:
1. Start the server in one terminal:
    ```
    sudo ./server
    ```
2. Start the client in another terminal with a query type:
    ```
    sudo ./client 1
    ```
3. Observe the server announcing its presence and responding to client queries.

### Sample Output:

#### Server:
Received QUERY from - 192.168.0.3
Received QUERY from - 192.168.0.3

#### Client:
192.168.0.3
Sent request - 2 - 36246

Response received - System Uptime: 2 days, 5 hours, 19 minutes, 20 seconds
Load Averages: 1 min: 0.58, 5 min: 0.85, 15 min: 0.76
Total RAM: 15676.57 MB
Free RAM: 4735.45 MB
Shared RAM: 683.08 MB
Buffered RAM: 617.77 MB
Total Swap: 0.00 MB
Free Swap: 0.00 MB
Number of Processes: 1462
192.168.0.3
Sent request - 2 - 36246

---

## Limitations and Assumptions

1. **Hardcoded IP Address:** The server and client uses a hardcoded IP (`192.168.0.3`) for its source address.
2. **Broadcast Scope:** HELLO packets are broadcast within the local subnet (`255.255.255.255`).
3. **No Encryption/Authentication:** The protocol does not include security features.
4. **Root Privileges Required:** Raw sockets require elevated privileges to run.

---

## Protocol Specification

### Message Types:

| Type   | Name      | Description                     |
|--------|-----------|---------------------------------|
| `0x01` | HELLO     | Node announcement               |
| `0x02` | QUERY     | Request for metadata            |
| `0x03` | RESPONSE  | Response with requested metadata|

---
