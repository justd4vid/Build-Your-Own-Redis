#ifndef COMMON_H
#define COMMON_H

#include <unistd.h>
#include <cstddef>
#include <cstdint>
#include <vector>

// Set socket to non-blocking read() & write()
void fd_set_nonblock(int fd);

// Append data to buffer
void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len);

// Remove data from buffer
void buf_consume(std::vector<uint8_t> &buf, size_t n);

// Read `n` bytes and store in buffer
int32_t read_full(int fd, char *buf, size_t n);

// Write `n` bytes from buffer
int32_t write_all(int fd, const char *buf, size_t n);

// Helper to print error and close socket. 
void die(const char *msg);

#endif