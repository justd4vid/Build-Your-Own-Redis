#include <iostream>
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
    
    int fd = socket(AF_INET, SOCK_STREAM, 0);                       // 1. Obtain a socket handle. We choose IPV4, TCP

    int val = 1;                                                    // 1.5. Set socket options. Generally good to set SO_REUSEADDR
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));    //    to allow server to use same IP:port immediately after restart. 

    struct sockaddr_in addr = {};                                   // 2. Bind to an address. We use wildcard 0.0.0.0:1234
    addr.sin_family = AF_INET;          // address family IPv4
    addr.sin_port = htons(1234);        // port 1234
    addr.sin_addr.s_addr = htonl(0);    // wildcard- accept connections anywhere
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {die("bind()");} // rv: 0 = success, -1 = error


    rv = listen(fd, SOMAXCONN);                                     // 3. Listen. Socket is created on listen(). SOMAXCONN is the 
    if (rv) die("listen()");                                        //    maximum queue size for pending connections. It's 4096 on linux.
                                                                    //    For modern use-case it has no affect, the queue never fills up.  

    while (true) {                                                  // 4. Accept connections. The server enters loop and processes each
        struct sockaddr_in client_addr = {};                        //    client connection. 
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen); // when client connects, return a new fd for that specific connection. 
        if(connfd < 0) {                                                    // The original fd keeps listening. 
            continue; // error
        }

        // Serve the one connection. 
        while(true) {
            int32_t err = one_request(connfd);
            if(err) {
                break;
            }
        }

        close(connfd);
    }
    
    return 0;
}