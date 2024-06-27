# Nom de l'exécutable
TARGET = Buckshot_Roulette

# Fichiers source
SRCS = main.c

# Compilateur et options de compilation
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11
LDFLAGS = -lSDL2 -lSDL2_image -lSDL2_ttf

# Règle par défaut (si vous tapez juste 'make')
all: $(TARGET)

# Règle pour créer l'exécutable
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

# Règle pour nettoyer les fichiers compilés
clean:
	rm -f $(TARGET)

# Règle pour exécuter le programme
run: $(TARGET)
	./$(TARGET)

# Indiquer que les règles 'clean' et 'run' ne sont pas des fichiers
.PHONY: all clean run
