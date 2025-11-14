# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17


all: Server Client 

Server: Server.cpp common.cpp
	$(CXX) $(CXXFLAGS) Server.cpp common.cpp -o Server

Client: Client.cpp common.cpp
	$(CXX) $(CXXFLAGS) Client.cpp common.cpp -o Client

clean:
	rm -f Server Client


.PHONY: all clean # tells make that these aren't actual files