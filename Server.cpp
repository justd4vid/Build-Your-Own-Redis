#include <iostream>
#include <cstring>      // for strerror()

#include <sys/socket.h> // for socket creation
#include <netinet/ip.h> 

#include <unistd.h>     // for read(), write(), close()

// Helper to print error and exit. 
void die(const char *msg) {
    std::cerr << msg << ": " << strerror(errno) << "\n";
    std::exit(EXIT_FAILURE);
}

int main() {
    std::cout << "Running Program: " << "\n";
    int fd = socket(AF_INET, SOCK_STREAM, 0);                       // 1. Obtain a socket handle. We choose IPV4, TCP

    int val = 1;                                                    // 2. Set socket options. Generally good to set SO_REUSEADDR
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));    //    to allow server to use same IP:port after restart. 

    struct sockaddr_in addr = {};                                   // 3. Bind to an address. We use wildcard 0.0.0.0:1234
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);        // port
    addr.sin_addr.s_addr = htonl(0);    // wildcard IP 0.0.0.0
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {die("bind()");}


    rv = listen(fd, SOMAXCONN);                                     // 4. Listen. Socket is created on listen(). SOMAXCONN is the size of 
    if (rv) {die("listen()");}                                      //    queue for max number of pending connections. It's 4096 on linux.
                                                                    //    For modern use-case it has no affect, the queue never fills up.  

    while (true) {                                                  // 5. Accept connections. The server enters loop and processes each
        struct sockaddr_in client_addr = {};                        //    client connection. 
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
        if(connfd < 0) {
            continue; // error
        }
        close(connfd);
    }




    
    return 0;
}