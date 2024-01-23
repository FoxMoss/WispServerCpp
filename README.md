# WispServerC++

A C++ project implementing the [wisp protocol](https://github.com/MercuryWorkshop/wisp-protocol), a minimal overhead websocket proxy.

This was made to take full advantage of the low overhead of the protocol by reducing memory usage compared to the author's implementation in python. The repo also has UDP support and better error error handling then the original implementation.

## Usage

`wispserver [port]`

## Install

Requirements:
- libboost
- libwebsocketpp

To make with debug symbols make with DEBUG set to TRUE
`make DEBUG="-g -ggdb"`
Otherwise symbols will be stripped.

The Makefile also needs a obj/ directory.

## Implementation Status

- [x] Base Packet Handling
    - [x] Connect Payload (partial)
        - [x] TCP
        - [x] UDP
    - [x] Client Data Payload
    - [x] Server Data Payload
    - [x] Continue Payload
    - [x] Close Payload (only necessary)
        - [x] 0x01 Unknown
        - [x] 0x02 Voluntary Closure (N/A)
        - [x] 0x03 Network Error
        - [x] 0x41 Creation Invalid
        - [x] 0x42 Host Unreachable
        - [x] 0x43 Creation Timeout
        - [x] 0x44 Connection Refused
        - [ ] 0x47 Tcp Timeout
        - [ ] 0x48 Server Blocked Host 
        - [ ] 0x49 Server Throttling
        - [x] 0x81 Client Memory Error (N/A)
