#include "common.h"
#include <iostream>
#include <cassert>
#include <cstring>

void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len) {
    buf.insert(buf.end(), data, data + len);
}

void buf_consume(std::vector<uint8_t> &buf, size_t n) {
    buf.erase(buf.begin(), buf.begin() + n);
}

void die(const char *msg) {
    std::cerr << "Error: " << msg << ": " << strerror(errno) << "\n";
    std::exit(EXIT_FAILURE);
}