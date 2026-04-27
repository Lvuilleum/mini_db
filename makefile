# Variables
CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iinclude
HEADERS = $(wildcard include/*.h)

# Fichiers sources partagés (utilisés par le serveur et potentiellement le client)
COMMON_SRC = src/parser.c

# Sources spécifiques
SERVER_SRC = src/db_server.c src/database.c src/storage.c $(COMMON_SRC)
CLIENT_SRC = src/db_client.c $(COMMON_SRC)

# Noms des exécutables
SERVER_TARGET = db_server
CLIENT_TARGET = db_client

# Règle par défaut : on compile les deux
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Compilation du serveur
$(SERVER_TARGET): $(SERVER_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_TARGET)

# Compilation du client
$(CLIENT_TARGET): $(CLIENT_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_TARGET)

# Pour lancer le serveur (dans un premier terminal)
run-server: $(SERVER_TARGET)
	./$(SERVER_TARGET)

# Pour lancer le client (dans un deuxième terminal)
run-client: $(CLIENT_TARGET)
	./$(CLIENT_TARGET)

clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)

.PHONY: all clean run-server run-client