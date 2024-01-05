CC = gcc
OBJ = main.o bmp.o files.o checksuma.o lzw.o png.o
LIBS = -L"lib" -lpng -lz
INCS = -I"include"
BIN = stegim.exe
CFLAGS = $(INCS) -Wall -Wextra

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN): $(OBJ)
	$(CC) $^ -o $@ $(LIBS)