# Variables
CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iinclude
SRC = $(wildcard src/*.c)
HEADERS = $(wildcard include/*.h)
TARGET = mydb

# Règle par défaut
all: $(TARGET)

# On dit à make que mydb dépend de tous les fichiers .c et .h
# Si l'un d'eux est plus récent que l'exécutable, il recompile.
$(TARGET): $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

# La règle run dépend de mydb : elle compilera avant de lancer si besoin
run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

# Empêche des conflits si un fichier s'appelle 'run' ou 'clean'
.PHONY: all run clean