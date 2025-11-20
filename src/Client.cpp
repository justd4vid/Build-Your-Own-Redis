#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>

#include "common.h"

const size_t k_max_msg = 32 << 20;

// ====== Lower-level read/write helpers ======
// Write `n` bytes from buffer
static int32_t write_all(int fd, const char *buf, size_t n) {
    while(n > 0) {
        ssize_t rv = write(fd, buf, n);
        if(rv <= 0) {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// Read `n` bytes and store in buffer
static int32_t read_full(int fd, uint8_t *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if(rv <= 0) {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// ====== Higher-level request-response functions ======
static int32_t send_request(int fd, const char *text, size_t len) {
    if(len > k_max_msg) {
        std::cout << "Error: cannot send message because it is too long." << "\n";
        return -1;
    }

    std::vector<char> wbuf(4 + len);
    memcpy(wbuf.data(), &len, 4);
    memcpy(wbuf.data() + 4, text, len);
    return write_all(fd, wbuf.data(), 4 + len);
}

static int32_t read_response(int fd) {
    // Read header
    std::vector<uint8_t> rbuf;
    rbuf.resize(4);
    errno = 0;
    int32_t err = read_full(fd, rbuf.data(), 4);
    if(err) {
        std::cout << (errno == 0 ? "EOF" : "read() header error") << "\n";
        return err;
    }

    uint32_t len;
    memcpy(&len, rbuf.data(), 4);
    if(len > k_max_msg) {
        std::cout << "Error: cannot receive message because it is too long." << "\n";
        return -1;
    }

    rbuf.resize(4 + len);
    err = read_full(fd, rbuf.data() + 4, len);
    if(err) {
        std::cout << "Error: read() body error" << "\n";
        return err;
    }
    printf("Server echoes: %.*s\n", len, rbuf.data() + 4);

    return 0;

}



int main() {
    std::cout << "==== Running Client ====" << "\n";

    // == Create listening socket ==
    int fd = socket(AF_INET, SOCK_STREAM, 0);                       // Select IPv4, TCP

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));    // Turn on SO_REUSEADDR

    struct sockaddr_in addr = {};                                   // Bind to address
    addr.sin_family = AF_INET;          // address family IPv4
    addr.sin_port = htons(1234);        // port 1234
    addr.sin_addr.s_addr = htonl(0);    // wildcard- accept connections anywhere

    int rv = connect(fd, (struct sockaddr *)&addr, sizeof(addr));   // Create socket
    if (rv) die("connect()");

    // == Write to server ==
    std::vector<std::string> query_list = {
        "test1", 
        "test2", 
        "test3000",
        std::string(k_max_msg, 'z')
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