# Simulated TCP Protocol Based on UDP

This project provides a simulated TCP protocol for reliable data transmission using UDP sockets. The implementation achieves a precise simulation of TCP congestion control, including slow start, fast recovery, and congestion avoidance states, while also handling packet delays and losses in data transmission.

## Features

- Simulated TCP protocol for reliable data transmission using UDP sockets
- Precise simulation of TCP congestion control, including slow start, fast recovery, and congestion avoidance states
- Handling of packet delays and losses in data transmission
- Bandwidth fairness between concurrent TCP connections, ensuring that the implementation can use ±50% of the bandwidth of a real TCP connection in the worst case.

## Usage

The project provides a set of APIs for data transmission, including functions for sending and receiving data, as well as for managing connections. Users can simply include the necessary headers and link to the library to use the simulated TCP protocol in their own projects.
