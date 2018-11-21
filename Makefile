DEVICE  = atmega328p
F_CPU   = 16000000 #2457600	# in Hz

CC = avr-gcc
CFLAGS = -Wall -Os -mmcu=$(DEVICE) -DF_CPU=$(F_CPU) -I.\
 -DLCD_PORT_NAME=D -DLCD_NCHARS=16

OBJ=main.o menu.o lcd.o time.o

main.bin: main.elf
	rm -f main.hex
	avr-objcopy -R .eeprom -O binary main.elf main.bin
	avr-size main.elf

main.elf: $(OBJ)
	$(CC) $(CFLAGS) -o main.elf -Wl,-Map,main.map $(OBJ)
#	$(CC) $(CFLAGS) -o main.elf -Wl,-Map,main.map,-u,vfprintf -lprintf_min $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o main.elf main.hex main.bin

load:
	sudo avrdude -P usb -c usbasp -p m32 -U flash:w:main.bin:r

fuse:
	sudo avrdude -P usb -c usbasp -p m32 -U lfuse:w:lfuse.bin:r -U hfuse:w:hfuse.bin:r

