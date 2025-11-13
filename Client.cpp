#include <iostream>
#include <cstring>

#include <sys/socket.h>
#include<netinet/ip.h>
#include <unistd.h>

static void die(const char *msg) {
    std::cerr << msg << ": " << strerror(errno) << "\n";
    std::exit(EXIT_FAILURE);
}

int main() {
    std::cout << "Running Client." << "\n";

    int fd = socket(AF_INET, SOCK_STREAM, 0);                       // 1. Obtain handle

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));    // 1.5. Set options

    struct sockaddr_in addr = {};                                   // 3. Connect. Socket is created. 
    addr.sin_family = AF_INET;          // address family IPv4
    addr.sin_port = htons(1234);        // port 1234
    addr.sin_addr.s_addr = htonl(0);    // wildcard- accept connections anywhere

    int rv = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rv) die("connect()");

    // Write to our socket
    char msg[] = "hello";
    write(fd, msg, strlen(msg));
    // Read from our socket
    char rbuf[64] = {};
    ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
    if(n < 0) die("read()");

    printf("Server says: %s\n", rbuf);

    close(fd);
    return 0;
}