C_FILE = main.c
FILES= $(C_FILE) io.h str.h complex_pipe.h
EXEC = main

all: $(EXEC)

$(EXEC): $(FILES)
	clang -g -std=c99 $(C_FILE) -o $(EXEC) 

clean:
	rm -f $(EXEC)
