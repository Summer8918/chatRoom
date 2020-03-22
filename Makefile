BINARIES = server cli

all:$(BINARIES)

CXXFLAGS = -O0 -g -Wall 

$(BINARIES):
	g++ $(CXXFLAGS) -o $@  $(filter %.c,$^)

clean:
	rm -f $(BINARIES) core

server: server.c
cli: client.c
