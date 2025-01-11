#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Event handler constants
#define MAX_EVENTS 128

typedef void aeFileProc(int fd, void *clientData);

// Event structure
typedef struct aeFileEvent {
    int mask;              
    aeFileProc *rfileProc;  
    aeFileProc *wfileProc;  
    void *clientData;       
} aeFileEvent;

// Event loop structure
typedef struct aeEventLoop {
    int epoll_fd;                
    struct epoll_event *events;  
    aeFileEvent *fileEvents;      
    int maxfd;                    
} aeEventLoop;

// Create the event loop
aeEventLoop *aeCreateEventLoop(int maxfd) {
    aeEventLoop *eventLoop = malloc(sizeof(aeEventLoop));
    if (!eventLoop) return NULL;
    eventLoop->epoll_fd = epoll_create(1024);
    if (eventLoop->epoll_fd == -1) {
        free(eventLoop);
        return NULL;
    }
    eventLoop->events = malloc(sizeof(struct epoll_event) * MAX_EVENTS);
    eventLoop->fileEvents = calloc(maxfd, sizeof(aeFileEvent));
    eventLoop->maxfd = maxfd;
    return eventLoop;
}

// Register file events
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
                      aeFileProc *proc, void *clientData) {
    struct epoll_event ee = {0};
    int op = eventLoop->fileEvents[fd].mask == 0 ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    // Set epoll event flags
    if (mask & EPOLLIN) ee.events |= EPOLLIN;
    if (mask & EPOLLOUT) ee.events |= EPOLLOUT;
    ee.data.fd = fd;

    // Update file event data
    aeFileEvent *fe = &eventLoop->fileEvents[fd];
    fe->mask |= mask;
    if (mask & EPOLLIN) fe->rfileProc = proc;
    if (mask & EPOLLOUT) fe->wfileProc = proc;
    fe->clientData = clientData;

    // Register with epoll
    if (epoll_ctl(eventLoop->epoll_fd, op, fd, &ee) == -1) {
        perror("epoll_ctl");
        return -1;
    }
    return 0;
}

// Poll for events
void aeProcessEvents(aeEventLoop *eventLoop) {
    int numevents = epoll_wait(eventLoop->epoll_fd, eventLoop->events, MAX_EVENTS, -1);
    for (int i = 0; i < numevents; i++) {
        struct epoll_event *e = &eventLoop->events[i];
        int fd = e->data.fd;
        aeFileEvent *fe = &eventLoop->fileEvents[fd];

        // Handle readable events
        if ((e->events & EPOLLIN) && fe->rfileProc) {
            fe->rfileProc(fd, fe->clientData);
        }
        // Handle writable events
        if ((e->events & EPOLLOUT) && fe->wfileProc) {
            fe->wfileProc(fd, fe->clientData);
        }
    }
}

// Close event loop
void aeDeleteEventLoop(aeEventLoop *eventLoop) {
    close(eventLoop->epoll_fd);
    free(eventLoop->events);
    free(eventLoop->fileEvents);
    free(eventLoop);
}

// Example usage: read/write handlers
void handleRead(int fd, void *clientData) {
    char buf[1024];
    int n = read(fd, buf, sizeof(buf));
    if (n > 0) {
        printf("Received: %.*s\n", n, buf);
        write(fd, buf, n); // Echo back
    } else {
        if (n == 0) {
            printf("Client disconnected: fd=%d\n", fd);
        } else {
            perror("read");
        }
        close(fd);
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(9090),
        .sin_addr.s_addr = INADDR_ANY,
    };
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 128);

    aeEventLoop *eventLoop = aeCreateEventLoop(1024);
    aeCreateFileEvent(eventLoop, server_fd, EPOLLIN, handleRead, NULL);

    while (1) {
        aeProcessEvents(eventLoop);
    }

    aeDeleteEventLoop(eventLoop);
    close(server_fd);
    return 0;
}
