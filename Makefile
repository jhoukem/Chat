# définition des cibles particulières
.PHONY: clean, mrproper
# désactivation des règles implicites
.SUFFIXES:
# définition des variables
CC = gcc
CFLAGS = -W -Wall
CFLAGS_T = -pthread
LDFLAGS = -lncurses
#SSLFLAGS=  -I/usr/include/openssl -L/usr/lib/x86_64-linux-gnu -lcrypto -lssl3 -lssl
# Le nom de l’exécutable à fabriquer
EXE1=server
EXE2=client

# all
all: $(EXE1) $(EXE2)

$(EXE1): server.o socket.o util.o
	 $(CC) $^ $(CFLAGS) $(CFLAGS_T) -o $@

$(EXE2): client.o socket.o sig.o util.o
	 $(CC) $^ $(CFLAGS) $(LDFLAGS) $(CFLAGS_T) -o $@

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

# clean
clean:
	rm -rf *.bak rm -rf *.o rm rm *~
# mrproper
mrproper: clean
	rm -rf $(EXE1) $(EXE2) 
