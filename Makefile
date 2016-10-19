# définition des cibles particulières
.PHONY: clean, mrproper
# désactivation des règles implicites
.SUFFIXES:
# définition des variables
CC = gcc
CFLAGS = -W -pthread

# Le nom de l’exécutable à fabriquer
EXE1=server
EXE2=client

# all
all: $(EXE1) $(EXE2)

$(EXE1): server.o socket.o
	 $(CC) $^ $(CFLAGS) -o $@

$(EXE2): client.o socket.o
	 $(CC) $^ $(CFLAGS) -o $@

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

# clean
clean:
	rm -rf *.bak rm -rf *.o rm rm *~
# mrproper
mrproper: clean
	rm -rf $(EXE1) $(EXE2) 
