CFLAGS+= -Wall -D__DEBUG -O2 -mtune=cortex-a8 -march=armv7-a
LDFLAGS+= -lprussdrv -lpthread

TARGET = pru_eblink
OBJ = $(TARGET).o
ASM = $(TARGET).p

all: $(TARGET) $(TARGET).bin

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $< 

$(TARGET): $(OBJ)
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET).bin: $(ASM)
	pasm -V3 -b $<

halt:	all
	./$(TARGET) h

run:	setup.js all
	node setup.js
	./$(TARGET) f

.PHONY: clean

clean:
	rm -f *.o *~  $(TARGET)

pruclean:
	rm -f $(TARGET).bin
