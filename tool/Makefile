
PLATFORM ?= ''

PROJDIRS := fatfs tool

SRCFILES := $(shell find $(PROJDIRS) -type f -name "*.c" ! -path "*/platform/*" )
ifneq ($(PLATFORM), '')
	SRCFILES += $(shell find ./tool/platform/$(PLATFORM) -type f -name "*.c")
endif
OBJFILES := $(patsubst %.c, %.o, $(SRCFILES))

.PHONY: all clean

WARNINGS :=
CFLAGS := -std=c23 $(WARNINGS) -I fatfs/source -fsanitize=address -g
LDFLAGS := -lasan -lext2fs -ltar -lz -linih -lmagic -lcurl -larchive

all: image

image: $(OBJFILES)
	@echo $(SRCFILES)
	@echo $(OBJFILES)
	@echo $(CC) $(LDFLAGS) -o image $?
	@$(CC) $(LDFLAGS) -o image $?

clean:
	-@$(RM) $(wildcard $(OBJFILES) image)

%.o: %.c
	@echo $(CC) $(CFLAGS) -c $< -o $@
	@$(CC) $(CFLAGS) -c $< -o $@
