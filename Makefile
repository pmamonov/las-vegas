DEVICE  = atmega32
F_CPU   = 2457600	# in Hz

CC = avr-gcc
CFLAGS = -Wall -Os -mmcu=$(DEVICE) -DF_CPU=$(F_CPU) -I.\
 -DLCD_PORT_NAME=A -DLCD_NCHARS=16

OBJ=main.o menu.o lcd.o

main.hex: main.elf
	rm -f main.hex
	avr-objcopy -R .eeprom -O ihex main.elf main.hex
	avr-size main.elf

main.elf: $(OBJ)
	$(CC) -o main.elf -Wl,-Map,main.map $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o main.elf main.hex 
