TARGET = myiop

CC = arm-none-linux-gnueabi-gcc

CFLAGS = -Wall -static

all: $(TARGET)

UTL_func.o:UTL_func.c UTL_func.h
	$(CC) -o $@ $< -c

IOP_packet.o:IOP_packet.c IOP_packet.h
	$(CC) -o $@ $< -c

myiop:myiop.c UTL_func.o IOP_packet.o
	$(CC) -o $@ $^ $(CFLAGS)

up: $(TARGET)
	adb push myiop /system

clean:
	rm -rf $(TARGET) *.o
