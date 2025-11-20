#ifndef COMMON_H
#define COMMON_H

#include <unistd.h>
#include <cstddef>
#include <cstdint>
#include <vector>

// Append data to buffer
void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len);

// Remove data from buffer
void buf_consume(std::vector<uint8_t> &buf, size_t n);

// Helper to print error and close socket. 
void die(const char *msg);

#endif