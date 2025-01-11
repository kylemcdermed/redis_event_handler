# Redis Event Handler
This repository provides an event-driven server implemented using `libuv`, 
inspired by Redis's event handling mechanism. 
The server processes incoming client connections, handles messages, 
and writes responses to sockets, demonstrating concurrency and non-blocking IO.

## Features
- Event-based handling of client connections using `libuv`.
- Processes and responds to client messages.
- Implements a custom state machine for message parsing.
- Inspired by Redis's event handling and file descriptor management.


