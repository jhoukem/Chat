# définition des cibles particulières
.PHONY: clean, mrproper
# désactivation des règles implicites
.SUFFIXES:
# définition des variables
CC = gcc
CFLAGS = -W -pthread

# Le nom de l’exécutable à fabriquer
EXE=server

# all
all: $(EXE)

client : client.o socket.o
	 $(CC) $^ $(CFLAGS) -o $@
$(EXE): server.o socket.o
	 $(CC) $^ $(CFLAGS) -o $@

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

# clean
clean:
	rm -rf *.bak rm -rf *.o rm rm *~
# mrproper
mrproper: clean
	rm -rf $(EXE) 
