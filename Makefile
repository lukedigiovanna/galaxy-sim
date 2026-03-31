CC     = gcc
CFLAGS = -Wall -std=c99 -O2

ifeq ($(OS),Windows_NT)
	TARGET = riftwalker.exe
	LIBS   = -lraylib -lopengl32 -lgdi32 -lwinmm
else
	TARGET = riftwalker
	UNAME  := $(shell uname -s)
	ifeq ($(UNAME),Darwin)
		LIBS = -lraylib -framework OpenGL -framework Cocoa \
		       -framework IOKit -framework CoreAudio -framework CoreVideo
	else
		LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
	endif
endif

SRC = src/main.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f riftwalker riftwalker.exe
