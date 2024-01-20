# WispServerC++

A C++ project implementing the [wisp protocol](https://github.com/MercuryWorkshop/wisp-protocol), a minimal overhead websocket proxy.

This was made to take full advantage of the low overhead of the protocol by reducing memory usage compared to the author's implementation in python.

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
    - [ ] Connect Payload (partial)
        - [x] TCP
        - [ ] UDP (untested)
    - [x] Client Data Payload
    - [x] Server Data Payload
    - [x] Continue Payload
    - [ ] Close Payload (partial)
        - [x] 0x01
        - [x] 0x02
        - [ ] 0x03
        - [ ] 0x41
        - [ ] 0x42
        - [ ] 0x43
        - [ ] 0x44
        - [ ] 0x45
        - [ ] 0x46
        - [ ] 0x47
        - [ ] 0x48
        - [ ] 0x49
        - [ ] 0x89
