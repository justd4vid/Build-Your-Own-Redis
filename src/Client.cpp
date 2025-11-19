#include <iostream>
#include <cstring>
#include <cassert>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <vector>

#include "common.h"

static int32_t send_request(int fd, const char *text, size_t vector_len) {
    uint32_t len = (uint32_t) vector_len;
    if(len > SOMAXCONN) {
        std::cout << "Error: cannot send message because it is too long." << "\n";
        return -1;
    }
    char wbuf[4 + len];
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], text, len);
    return write_all(fd, wbuf, 4 + len);
}

static int32_t read_response(int fd) {
    // Read header
    char rbuf[4 + SOMAXCONN];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if(err) {
        std::cout << (errno == 0 ? "EOF" : "read() header error") << "\n";
        return err;
    }

    uint32_t len;
    memcpy(&len, rbuf, 4);
    if(len > SOMAXCONN) {
        std::cout << "Error: cannot receive message because it is too long." << "\n";
        return -1;
    }

    err = read_full(fd, &rbuf[4], len);
    if(err) {
        std::cout << "Error: read() body error" << "\n";
        return err;
    }
    printf("Server echoes: %.*s\n", len, &rbuf[4]);

    return 0;

}



int main() {
    std::cout << "Running Client." << "\n";

    int fd = socket(AF_INET, SOCK_STREAM, 0);                       // Select IPv4, TCP

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));    // 1.5. Set options

    struct sockaddr_in addr = {};                                   // 3. Connect. Socket is created. 
    addr.sin_family = AF_INET;          // address family IPv4
    addr.sin_port = htons(1234);        // port 1234
    addr.sin_addr.s_addr = htonl(0);    // wildcard- accept connections anywhere

    int rv = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rv) die("connect()");

    // Write to our socket
    std::vector<std::string> query_list = {
        "test1", "test2", "test3000",
    };
    for (const std::string &s : query_list) {
        int32_t err = send_request(fd, s.data(), s.size());
        if (err) { 
            std::cout << "Send_request failed for query string: " << s << "\n";
        }
    }
    for (size_t i = 0; i < query_list.size(); ++i) {
        int32_t err = read_response(fd);
        if (err) {
            std::cout << "Read_response failed for query string: " << query_list[i] << "\n";
        }
    }

    close(fd);
    return 0;
}