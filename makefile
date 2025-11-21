# Compilateur
CC = gcc
CFLAGS = -Wall -Wextra -g

# Liste automatique de tous les .cpp
SOURCES = $(wildcard *.cpp)

# Chaque source donne un exécutable sans extension
EXECUTABLES = $(SOURCES:.cpp=)

all: $(EXECUTABLES)

# Règle générique : fichier.cpp → fichier
%: %.cpp
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o

mrproper: clean
	rm -f $(EXECUTABLES)

