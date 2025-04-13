# Name of the output binary
TARGET = ventilator

# Source file
SRC = ventilator.c

# Compiler and flags
CC = clang
CFLAGS = -Wall -Wextra -pedantic -std=gnu17
LDFLAGS = -lrt   # POSIX message queues require linking with librt

# Default build target
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Clean target to remove build artifacts
clean:
	rm -f $(TARGET)

# Optional run target with arguments
run: $(TARGET)
	./$(TARGET) -w 3 -t 10 -s 5
