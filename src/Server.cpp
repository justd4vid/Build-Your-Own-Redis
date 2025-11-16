#include <iostream>
#include <vector>
#include <cstring>      // for strerror()
#include <cassert>

#include <sys/socket.h> // for socket creation
#include <netinet/ip.h> // for internal protocol addresses
#include <unistd.h>     // for read(), write(), close()

#include "common.h"

// Read header + body. Write response.
static int32_t one_request(int connfd) {
    // == Read from Client ==
    char rbuf[4 + SOMAXCONN];
    errno = 0; // set by sys if fails

    // Read header (4 bytes)
    int32_t err = read_full(connfd, rbuf, 4);
    if(err) {
        msg(errno == 0 ? "EOF" : "read() header error");
        return err;
    }
    uint32_t len; // Header payload: how many bytes in body
    memcpy(&len, rbuf, 4); // conver header to int; assume little endian
    if(len > SOMAXCONN) {
        msg("Cannot receive message: too long.");
        return -1;
    }

    // Read request Body
    err = read_full(connfd, &rbuf[4], len);
    if(err) {
        msg("read() body error");
        return err;
    }
    printf("Client says: %.*s\n", len, &rbuf[4]);
    
    // == Write to Client ==
    const char reply[] = "Message received.";
    len = (uint32_t)strlen(reply);

    char wbuf[4 + sizeof(reply)];
    memcpy(wbuf, &len, 4); // create header
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}

int main() {
    std::cout << "Running Server: " << "\n";

    // ==== Create listening socket ====

    int fd = socket(AF_INET, SOCK_STREAM, 0);                       // 1. Obtain a socket handle. We choose IPV4, TCP

    int val = 1;                                                    // 1.5. Set socket option SO_REUSEADDR to allow server
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));    //      to use same IP:port immediately after restart. 

    struct sockaddr_in addr = {};                                   // 2. Bind to an address. We use wildcard 0.0.0.0:1234
    addr.sin_family = AF_INET;          // address family IPv4
    addr.sin_port = htons(1234);        // port 1234
    addr.sin_addr.s_addr = htonl(0);    // wildcard- accept connections anywhere
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {die("bind()");} // rv: 0 = success, -1 = error

    rv = listen(fd, SOMAXCONN);                                     // 3. Listen. Socket is created on listen(). SOMAXCONN is the 
    if (rv) die("listen()");                                        //    maximum queue size for pending connections. It's 4096 on linux.
                                                                    //    For modern use-case it has no affect, the queue never fills up.  

    // ==== Event loop ====
    std::vector<Conn *> fd2Conn; // map of client connections, keyed by fd
    std::vector<struct pollfd> poll_args;
    while(true) {
        // 1. Prepare the arguments of poll()
        poll_args.clear();
        // place the listening socket in first position
        struct pollfd pfd = {fd, POLLIN, 0}; // args: fd, event, revents
        poll_args.push_back(pfd);

        // place connection sockets after
        for (Conn *conn : fd2Conn) {
            if(!conn) {
                continue;
            }
            struct pollfd pfd = {conn_fd, POLLERR, 0};
            if(conn->want_read) {
                pfd.events |= POLLIN;
            }
            if(conn->want_write) {
                pfd.events |= POLLOUT;
            }
            poll_args.push_back(pfd)
        }

        // 2. Wait for readiness with poll()
        int rv = poll(poll_args.data(), (nfds_t) poll_args.size(), -1) {
            if (rv < 0 && errno == EINTR) {
                continue; // EINTR means syscall was interrupted. not an error
            }
            if(rv < 0) {
                die("poll() error");
            }
        }

        // 3. Handle new connections with the listening socket
        if(poll_args[0].revents) { 
            if(Conn *conn = handle_accept(fd)) {
                // put into map
                if(fd2conn.size() <= (size_t) conn->fd) {
                    f2dconn.resize(conn->fd+1);
                }
                fd2conn[conn->fd] = conn;
            }
        }

        // 4. Invoke callbacks for connection sockets. 
        for(size_t i = 1;i<poll_args.size();i++) {
            // Get status and connection
            uint32_t ready = poll_args[i].revents;
            Conn *conn = fd2conn[poll_args[i].fd];
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
                fd2conn[conn->fd] = NULL;
                delete conn; // needed since we create `Conn` object with `new`
            }
        }

    }


    // while (true) {                                                  // 4. Accept connections. The server enters loop and processes each
    //     struct sockaddr_in client_addr = {};                        //    client connection. 
    //     socklen_t addrlen = sizeof(client_addr);
    //     int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen); // when client connects, return a new fd for that specific connection. 
    //     if(connfd < 0) {                                                    // The original fd keeps listening. 
    //         continue; // error
    //     }

    //     // Serve the one connection. 
    //     while(true) {
    //         int32_t err = one_request(connfd);
    //         if(err) {
    //             break;
    //         }
    //     }

    //     close(connfd);
    // }
    
    return 0;
}