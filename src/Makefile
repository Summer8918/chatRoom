LIB_SRC = client.cpp server.cpp
BINARIES = cli server2

all:$(BINARIES)

CXXFLAGS = -O0 -g -Wall -l pthread 

$(BINARIES):
	g++ $(CXXFLAGS) -o $@ $(LIB_SRC) $(filter %.cpp,$^)

clean:
	rm -f $(BINARIES) core

#server: server.cpp
cli: client_main.cpp
server2: server_main.cpp