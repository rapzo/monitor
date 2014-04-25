CC = gcc
CFLAGS = -Wall
APP = monitor
rm = rm -f
DBG = -g

SRCDIR = src
OBJDIR = obj
BINDIR = bin
SOURCES := $(wildcard $(SRCDIR)/*.c)
OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

$(BINDIR)/$(APP): $(OBJECTS)
	$(CC) -o $@ $^
	@echo "Compiling complete! Use following the example to run the application."
	@echo "./$@ <duration in seconds> <word to look for <file 1> \
	<file 2> ... <file n>"

$(OBJDIR)/dldl.o: $(SRCDIR)/dldl.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/screener.o: $(SRCDIR)/screener.c
	$(CC) $(CFLAGS) -c $< -o $@

debug: src/screener.c
	$(CC) $(DBG) $<

clean:
	@$(rm) $(OBJDIR)/*.o $(BINDIR)/$(APP)

.PHONY: clean
