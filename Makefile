CC     = gcc
CFLAGS = -Wall -std=c99 -O2

ifeq ($(OS),Windows_NT)
	TARGET = galaxy-sim.exe
	LIBS   = -lraylib -lopengl32 -lgdi32 -lwinmm
else
	TARGET = galaxy-sim
	UNAME  := $(shell uname -s)
	ifeq ($(UNAME),Darwin)
		LIBS = -lraylib -framework OpenGL -framework Cocoa \
		       -framework IOKit -framework CoreAudio -framework CoreVideo
	else
		LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
	endif
endif

SRC = src/main.c       \
      src/simulation.c  \
      src/galaxy_init.c \
      src/renderer.c    \
      src/camera.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f galaxy-sim galaxy-sim.exe
