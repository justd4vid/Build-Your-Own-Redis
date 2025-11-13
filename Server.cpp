#include <iostream>
#include <cstring>      // for strerror()

#include <sys/socket.h> // for socket creation
#include <netinet/ip.h> // for internal protocol addresses
#include <unistd.h>     // for read(), write(), close()

// Helper to print error and exit. 
static void die(const char *msg) {
    std::cerr << msg << ": " << strerror(errno) << "\n";
    std::exit(EXIT_FAILURE);
}

// Helper to print message. 
static void msg(const char *msg) {
    std::cout << msg << "\n";
}

static void dummy_process(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1); // read rbuf size chars!
    if(n==0) {
        msg("Client closed connection.");
        return;
    }else if(n<0) {
        msg("read() error");
        return;
    }
    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
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
    if (rv) die("listen()");                                      //    maximum queue size for pending connections. It's 4096 on linux.
                                                                    //    For modern use-case it has no affect, the queue never fills up.  

    while (true) {                                                  // 4. Accept connections. The server enters loop and processes each
        struct sockaddr_in client_addr = {};                        //    client connection. 
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen); // when client connects, return a new fd for that specific connection. 
        if(connfd < 0) {                                                    // The original fd keeps listening. 
            continue; // error
        }

        dummy_process(connfd);

        close(connfd);
    }
    
    return 0;
}