# more info on makefiles 
# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CC       := gcc
CFLAGS   := -std=c99 -Wall
# c99 standar, show a lot of warnings

SRCDIR   := src
BUILDDIR := build
SRCEXT   := c # change to cc or cpp in a c++ project
TARGET   := bin/sfserver

SOURCES  := $(shell find $(SRCDIR) -type f -name *.c)
OBJECTS  := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.c=.o))

INC := -I $(SRCDIR)
LIB := #-lncurses

$(TARGET): $(OBJECTS)
	@mkdir -p bin
	@echo " Link libraries..."
	@echo " $(CC) $^ -o $(TARGET) $(LIB)"; $(CC) $^ -o $(TARGET) $(LIB)

# $@ files named on the left side, $< is the first dependency
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo " Doing some cleanup..."
	@echo " rm -r $(BUILDDIR) $(TARGET)"; rm -r $(BUILDDIR) $(TARGET)

.PHONY: clean
