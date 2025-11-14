#include <iostream>
#include <cstring>
#include <cassert>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>

#include "common.h"

static int32_t query(int fd, const char *text) {
    uint32_t len = (uint32_t) strlen(text); // calling strlen on C-style string
    if(len > SOMAXCONN) {
        msg("write() longer than SOMAXCONN.");
        return -1;
    }
    
    // == Write to Server ==
    char wbuf[4 + len];
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], text, len);
    write_all(fd, wbuf, 4 + len);
    
    // == Read from Server ==
    char rbuf[4 + SOMAXCONN];
    errno = 0; // set by sys if fails

    // Read header (4 bytes)
    int32_t err = read_full(fd, rbuf, 4);
    if(err) {
        msg(errno == 0 ? "EOF" : "read() header error");
        return err;
    }
    memcpy(&len, rbuf, 4); // conver header to int; assume little endian
    if(len > SOMAXCONN) {
        msg("Cannot receive message: too long.");
        return -1;
    }

    // Read request Body
    err = read_full(fd, &rbuf[4], len);
    if(err) {
        msg("read() body error");
        return err;
    }
    printf("Server says: %.*s\n", len, &rbuf[4]);

    return 0;
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
    int32_t err = query(fd, "Hello. This is the client.");
    if(err) {
        close(fd);
        return 0;
    }

    err = query(fd, "Another message from the client!");
    if(err) {
        close(fd);
        return 0;
    }

    close(fd);
    return 0;
}