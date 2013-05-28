VERSION = 0.1

CC = ack -mrpi -O
hide = @

SRCS = \
	src/main.c \
	src/mmc.c \
	src/parser.c

CFLAGS = -DVERSION=\"$(VERSION)\"

OBJS = $(patsubst %.c,%.o,$(SRCS))

all: piface.bin

%.o: %.c
	@echo CC $@
	$(hide) $(CC) $(CFLAGS) $< -c -o $@

piface.bin: $(OBJS)
	@echo LINK $@
	$(hide) $(CC) -t -.c -o $@ $(OBJS)

clean:
	@echo CLEAN
	$(hide) rm -f $(OBJS) piface.bin
