# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -Wpedantic -Werror -std=c++17


all: Server Client 

Server: src/Server.cpp src/common.cpp src/common.h
	$(CXX) $(CXXFLAGS) src/Server.cpp src/common.cpp -o Server

Client: src/Client.cpp src/common.cpp src/common.h
	$(CXX) $(CXXFLAGS) src/Client.cpp src/common.cpp -o Client

clean:
	rm -f Server Client


.PHONY: all clean # tells make that these aren't actual files