#ifndef COMMON_H
#define COMMON_H

#include <unistd.h>
#include <cstddef>
#include <cstdint>

// Read `n` bytes and store in buffer
int32_t read_full(int fd, char *buf, size_t n);

// Write `n` bytes from buffer
int32_t write_all(int fd, const char *buf, size_t n);

// Helper to print message. 
void msg(const char *msg);

// Helper to print error and close socket. 
void die(const char *msg);

#endif