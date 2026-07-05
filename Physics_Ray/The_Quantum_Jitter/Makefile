CC      = gcc
CFLAGS  = -O2 -Wall -std=c11
TARGET  = alqc_gpu
SRC     = alqc_gpu.c
LIBS    = -lGL -lGLEW -lglfw -lm

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: clean
