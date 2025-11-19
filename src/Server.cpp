#include <iostream>
#include <vector>
#include <cstring>      // for strerror()
#include <cassert>

#include <sys/socket.h> // for socket creation
#include <netinet/ip.h> // for internal protocol addresses
#include <unistd.h>     // for read(), write(), close()
#include <poll.h>

#include "common.h"

// ====== Connection Helpers ======

// <summary>
// Wraps a socket. Includes status flags and in/out buffers. 
// </summary>
struct Conn {
    int fd = -1;
    bool want_read = false;
    bool want_write = false;
    bool want_close = false;
    // buffered input and output
    std::vector<uint8_t> incoming;  // data incoming to server
    std::vector<uint8_t> outgoing;  // data outgoing from server
};

// Helper: set up a new connection.
static Conn *handle_accept(int fd) {
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);

    int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
    if(connfd < 0) {
        return NULL; // error
    }
    fd_set_nonblock(connfd);

    // Create Connection obj for the raw fd
    Conn *conn = new Conn();
    conn->fd = connfd;
    conn->want_read = true;
    return conn;
}

// ====== read() & write() Helpers ======

// Helper: print received bytes as chars
static void print_bytes_as_chars(const uint8_t *bytes, int32_t len) {
    std::cout << "Received: ";
    for (int32_t i=0;i<len;i++) {
        std::cout << static_cast<char>(bytes[i]);
    }

    std::cout << "\n";
}

// Helper for handle_read().
// Executes one request-response action by 
// reading header + body, then echoing back. 
static int32_t try_one_request(Conn *conn) {
    // == Read header ==
    if (conn->incoming.size() < 4) {
        return false;
    }
    uint32_t len = 0;
    memcpy(&len, conn->incoming.data(), 4);
    if(len > SOMAXCONN) { // Protocol broken: request too long. 
        conn->want_close = true;
        return false;
    }

    // == Read body ==
    if(4 + len > conn->incoming.size()) { // Protocol broken: expected body too short. 
        return false;
    }
    const uint8_t *request_body = &conn->incoming[4];

    // == Generate response (echo) ==
    print_bytes_as_chars(request_body, len);                     // Print received bytes

    buf_append(conn->outgoing, (const uint8_t *)&len, 4);   // Append header
    buf_append(conn->outgoing, request_body, len);               // Append body
    
    buf_consume(conn->incoming, 4 + len);                   // Consume the read bytes
    
    return true; // success
}

// Helper: called when conn->want_read is true.
static void handle_read(Conn *conn) {
    // 1. Non-blocking read. Dump into our Conn buffer. 
    uint8_t buf[64*1024]; // 64KB is common size for network I/O. larger than a typical read()
    ssize_t rv = read(conn->fd, buf, sizeof(buf));
    if(rv <=0 ) {  // handle error or EOF
        conn->want_close = true;
        return;
    }
    buf_append(conn->incoming, buf, (size_t)rv);

    // Make a request back
    while(try_one_request(conn)) {
    }

    if(conn->outgoing.size() > 0) { // Stop reads to send a response back. 
        conn->want_read = false;
        conn->want_write = true;
    } // else: go back to reading.
}

// Helper: called when conn->want_write == true.
static void handle_write(Conn *conn) {
    assert(conn->outgoing.size() > 0);
    ssize_t rv = write(conn->fd, conn->outgoing.data(), conn->outgoing.size());
    if (rv < 0 && errno == EAGAIN) {
        return; // client socket not ready
    }
    if(rv < 0) {
        conn->want_close = true; 
        return;
    }

    buf_consume(conn->outgoing, (size_t) rv);

    if(conn->outgoing.size() == 0) {
        conn->want_read = true;
        conn->want_write = false;
    } // else: want to continue writing. 
}

int main() {
    std::cout << "==== Running Server ====" << "\n";

    // ==== Create listening socket ====
    int fd = socket(AF_INET, SOCK_STREAM, 0);                           // Select IPv4, TCP

    int val = 1;                                                        // Turn on SO_REUSEADDR to allow server to use 
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));        //   same IP:port immediately after restart. 
    fd_set_nonblock(fd);

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;          // address family IPv4
    addr.sin_port = htons(1234);        // port 1234
    addr.sin_addr.s_addr = htonl(0);    // wildcard- accept any connections
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));    // Bind to address wildcard 0.0.0.0:1234
    if (rv) {die("bind()");} // rv: 0 = success, -1 = error

    rv = listen(fd, SOMAXCONN);                                         // Socket is created on listen(). SOMAXCONN is max 
    if (rv) die("listen()");                                            //   queue size for pending connections (4096 on linux)  


    // ==== Event loop ====
    std::vector<Conn *> fdToConn;           // Map of client connections, indexed with fd
    std::vector<struct pollfd> poll_args;   // Vector of args for poll()
    while(true) { 
        // 1. Prepare the args for poll()
        poll_args.clear();
        // place listening socket in first position
        struct pollfd pfd = {fd, POLLIN, 0}; // args: fd, event, revents
        poll_args.push_back(pfd);

        // place connection sockets after
        for (Conn *conn : fdToConn) {
            if(!conn) { // we may temporarily have pockets of NULL
                continue;
            }
            struct pollfd pfd = {conn->fd, 0, 0}; // fd, events, revents 
            if(conn->want_read) {
                pfd.events |= POLLIN;
            }
            if(conn->want_write) {
                pfd.events |= POLLOUT;
            }
            poll_args.push_back(pfd);
        }

        // 2. Wait for readiness with poll()
        int rv = poll(poll_args.data(), (nfds_t) poll_args.size(), -1); // timeout = -1 (none)
        if (rv < 0 && errno == EINTR) {
            continue; // EINTR means syscall was interrupted. not an error
        }else if(rv < 0) {
            die("poll() error");
        }

        // 3. Handle new connections with the listening socket
        if(poll_args[0].revents) { 
            if(Conn *conn = handle_accept(fd)) { // Conn created successfully
                if(fdToConn.size() <= (size_t) conn->fd) {
                    fdToConn.resize(conn->fd+1);
                }
                fdToConn[conn->fd] = conn;
            }
        }

        // 4. Invoke callbacks for connection sockets. 
        for(size_t i = 1; i<poll_args.size(); i++) {
            // Get status and connection
            uint32_t ready = poll_args[i].revents;
            Conn *conn = fdToConn[poll_args[i].fd];

            // Read & write
            if(ready & POLLIN) {
                handle_read(conn);
            }
            if(ready & POLLOUT) {
                handle_write(conn);
            }

            // Close connection if needed (potentially from error) 
            if((ready & POLLERR) || conn->want_close) {
                (void) close(conn->fd);
                fdToConn[conn->fd] = NULL;
                delete conn; // needed since we create `Conn` object with `new`
            }
        }

    }
    
    return 0;
}