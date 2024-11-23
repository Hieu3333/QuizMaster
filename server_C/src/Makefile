CC = gcc
CFLAGS = -Wall -g -I/usr/local/include/libmongoc-1.0 -I/usr/local/include/libbson-1.0 # Include paths for MongoDB and BSON
LIBS = -ljson-c -lmongoc-1.0 -lbson-1.0 # No need to link mongoose library since it's part of your source files

# Source files (added mongoose.c here)
SRC = auth.c mongodb_connect.c user.c server.c mongoose.c  # Added mongoose.c to the list of sources

# Object files (convert .c to .o)
OBJ = $(SRC:.c=.o)

# Executable name
EXEC = server

# Default target: Build and run the server
all: $(EXEC)

# Rule to link object files and create the executable
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJ) $(LIBS)

# Rule to compile .c files into .o object files
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# Clean up object files and the executable
clean:
	rm -f $(OBJ) $(EXEC)

# Run the server after compilation
run: $(EXEC)
	./$(EXEC)

# Clean, rebuild, and run
rebuild: clean all run
