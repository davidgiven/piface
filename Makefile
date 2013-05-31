VERSION = 0.1

OBJDIR = .obj
hide = @

SRCS = \
	src/main.c \
	src/error.c \
	src/mmc.c \
	src/parser.c \
	src/vfs.c \
	src/vfs_mem.c \
	src/vfs_host.c \
	src/dump.c \
	src/xmodem.c

CFLAGS = -DVERSION=\"$(VERSION)\"

all: piface.bin piface

clean::
	@echo CLEAN

depends :=
define build-piface
$(eval objs := $(patsubst %.c,$(OBJDIR)/$(variant)/%.o,$(SRCS)))
$(eval depends += $(patsubst %.c,$(OBJDIR)/$(variant)/%.d,$(SRCS)))

$(OBJDIR)/$(variant)/%.o: %.c
	@echo CC $(variant) $$@
	@mkdir -p $$(dir $$@)
	$(hide) gcc $(CFLAGS) $(cflags) -MM -MQ $$@ -o $$(patsubst %.o,%.d,$$@) $$^
	$(hide) $(cc) $(CFLAGS) $(cflags) $$< -c -o $$@

$(variant): $(objs)
	@echo LINK $(variant) $$@
	@mkdir -p $$(dir $$@)
	$(hide) $(link) -o $$@ $$^

clean::
	$(hide) rm -f $(objs) $(variant)
endef

variant := piface.bin
cflags := -DTARGET_PI
cc := ack -mrpi -O
link := $(cc) -.c -t
$(eval $(build-piface))

variant := piface
cflags := -DTARGET_TESTBED
cc := gcc -g -Wall
link := $(cc)
$(eval $(build-piface))

-include $(depends)
clean::
	$(hide) rm -f $(depends)

