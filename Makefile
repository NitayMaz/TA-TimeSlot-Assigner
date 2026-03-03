CC = gcc
CFLAGS = -O2 -Wall
TARGET = scheduler

all: $(TARGET)

$(TARGET): main.c csv_parser.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
