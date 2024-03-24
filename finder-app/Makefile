SRC := writer.c
TARGET = writer
OBJS := $(SRC:.c=.o)

all: $(TARGET)

# if crosscompile is specified, prepend that to gcc
$(TARGET) : $(OBJS)
# $(TARGET) : writer.c
	$(CROSS_COMPILE)gcc $(SRC) -o $(TARGET)

clean:
	-rm -f *.o $(TARGET) *.elf *.map
