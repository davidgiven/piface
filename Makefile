VERSION = 0.1

OBJDIR = .obj
hide = @

SRCS = \
	src/main.c \
	src/mmc.c \
	src/parser.c

CFLAGS = -DVERSION=\"$(VERSION)\"

all: piface.bin piface

clean::
	@echo CLEAN

define build-piface
$(eval objs := $(patsubst %.c,$(OBJDIR)/$(variant)/%.o,$(SRCS)))

$(OBJDIR)/$(variant)/%.o: %.c
	@echo CC $$@
	@mkdir -p $$(dir $$@)
	$(hide) $(cc) $(CFLAGS) $(cflags) $$< -c -o $$@

$(variant): $(objs)
	@echo LINK $$@
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
cc := gcc
link := $(cc)
$(eval $(build-piface))
