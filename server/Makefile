SRC := aesdsocket.c
TARGET = aesdsocket
CFLAGS := -Wall -Werror -g
OBJS := $(SRC:.c=.o)
INCLUDES = -pthread

all: $(TARGET)

# if crosscompile is specified, prepend that to gcc
$(TARGET) : $(OBJS)
# $(TARGET) : writer.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	-rm -f *.o $(TARGET) *.elf *.map
