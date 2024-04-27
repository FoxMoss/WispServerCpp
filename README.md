# WispServerC++
A C++ project implementing the [wisp protocol](https://github.com/MercuryWorkshop/wisp-protocol), a minimal overhead websocket proxy. It is curently the *fastest* implementation according to [wispmark](https://github.com/MercuryWorkshop/wispmark).

This was made to take full advantage of the low overhead of the protocol by reducing memory usage compared to the [author's implementation in python](https://github.com/MercuryWorkshop/wisp-server-python/). The repo also has UDP support and better error error handling then the original implementation.

## See Also

Create a PR if you want you're wisp related product here

For node support look at out port of WispServerC++ [wisp-server-cpp](https://github.com/FoxMoss/wisp-server-cpp) and the native [wisp-server-node](https://github.com/MercuryWorkshop/wisp-server-node/).

More info at [MercuryWorkshop/wisp-protocol](https://github.com/MercuryWorkshop/wisp-protocol).

## Usage

```
Usage: wispserver [options] <port>
 --packet-ratelimit <limit>      The ammount of packets that can be sent from a single websocket per minute.
 --help                          Shows this help menu.
```

## Install

Requirements:
- pthreads
- uWebSockets

To make with debug symbols make with DEBUG set to TRUE
`make DEBUG="-g -ggdb"`
Otherwise symbols will be stripped.

The Makefile also needs a obj/ directory.

## Implementation Status

- [x] V1
    - [x] Connect Payload
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
        - [x] 0x49 Server Throttling
        - [x] 0x81 Client Memory Error (N/A)
- [x] V2
    - [x] Protocol Extension Parsing
        - [x] 0x01 UDP Extension 
        - [ ] 0x02 Password Authentication

# Credits
- [epoxy-tls](https://github.com/mercuryWorkshop/epoxy-tls) for testing
