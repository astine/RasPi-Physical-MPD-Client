CFLAGS = -Wall -g
LDFLAGS = -lwiringPi -lasound -lmpdclient
CC = gcc

main: lcd/lcd.o rotaryencoder/rotaryencoder.o button/button.o led/led.o
	$(CC) $(CFLAGS) $(LDFLAGS) main.c lcd/lcd.o rotaryencoder/rotaryencoder.o button/button.o led/led.o -o main

lcd/lcd.o: lcd/lcd.c
	$(CC) $(CFLAGS) -c lcd/lcd.c -o lcd/lcd.o

rotaryencoder/rotaryencoder.o: rotaryencoder/rotaryencoder.c
	$(CC) $(CFLAGS) -c rotaryencoder/rotaryencoder.c -o rotaryencoder/rotaryencoder.o

button/button.o: button/button.c
	$(CC) $(CFLAGS) -c button/button.c -o button/button.o

led/led.o: led/led.c
	$(CC) $(CFLAGS) -c led/led.c -o led/led.o

clean:
	rm -f main lcd/lcd.o rotaryencoder/rotaryencoder.o button/button.o led/led.o
